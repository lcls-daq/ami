#include "PimaxHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const unsigned offset=1<<16;

static inline unsigned height(const Pds::Pimax::ConfigV1& c)
{
  return c.height()/c.binY();
}

static unsigned width(const Pds::Pimax::ConfigV1& c)
{
  return c.width()/c.binX();
}

using namespace Ami;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_PimaxFrame);
  return types;
}

PimaxHandler::PimaxHandler(const Pds::DetInfo& info, FeatureCache& cache) :
  EventHandler(info, data_type_list(), Pds::TypeId::Id_PimaxConfig),
  _cache(cache),
  _iCacheIndexTemperature(-1),
  _entry        (0),
  _pentry       (0),
  _offset       (make_ndarray<unsigned>(1,1)),
  _options      (0)
{
}

//PimaxHandler::PimaxHandler(const Pds::DetInfo& info, const EntryImage* entry) :
//  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_PimaxConfig),
//  _entry(entry ? new EntryImage(entry->desc()) : 0)
//{
//}

PimaxHandler::~PimaxHandler()
{
  if (_pentry)
    delete _pentry;
}

unsigned PimaxHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* PimaxHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void PimaxHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void PimaxHandler::reset()
{
  _entry = 0;
  if (_pentry) {
    delete _pentry;
    _pentry=0;
  }
}

void PimaxHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
  if (type.version() == Pds::Pimax::ConfigV1::Version)
    _config = *reinterpret_cast<const Pds::Pimax::ConfigV1*>(payload);
  else
    printf("PimaxHandler::_configure(): Unsupported Pimax Version %d\n", type.version());

  unsigned columns = width (_config);
  unsigned rows    = height(_config);
  unsigned ppb     = image_ppbin(columns, rows);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  { DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns, rows, ppb, ppb);
    _entry  = new EntryImage(desc); }

  { DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns*ppb, rows*ppb, 1, 1);
    _pentry = new EntryImage(desc);
    _pentry->invalid(); }

  _load_pedestals();

  /*
   * Setup temperature variable
   */
  char sTemperatureVar[64];
  sprintf(sTemperatureVar, "Pimax-%d-T", det.devId());
  _iCacheIndexTemperature = _cache.add(sTemperatureVar);
}

void PimaxHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) {}

void PimaxHandler::_event(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.id() == Pds::TypeId::Id_PimaxFrame)
  {
    const Pds::Pimax::FrameV1& f = *reinterpret_cast<const Pds::Pimax::FrameV1*>(payload);
    if (!_entry) return;

    const DescImage& desc = _entry->desc();
    unsigned o = desc.options();
    if (_options != o) {
      printf("PimaxHandler options %x -> %x\n",_options,o);
      _options = desc.options();
    }

    if (desc.options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( desc.options()&~FrameCalib::option_reload_pedestal() );
    }

    const ndarray<const unsigned,2>& pa =
      desc.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pentry->content();

    int ppbin = _entry->desc().ppxbin();
    memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
    const uint16_t* d = reinterpret_cast<const uint16_t*>(f.data(_config).data());
    for(unsigned j=0; j<height(_config); j++) {
      const unsigned* p = &pa(j,0);
      for(unsigned k=0; k<width(_config); k++, d++, p++)
        _entry->addcontent(*d + *p, k/ppbin, j/ppbin);
    }

    //  _entry->info(f.offset()*ppbx*ppby,EntryImage::Pedestal);
    _entry->info(double(offset*ppbin*ppbin),EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);

    if (_iCacheIndexTemperature != -1 && f.temperature() != 999) // 999: temperature not defined
      _cache.cache(_iCacheIndexTemperature, f.temperature());
  }
}

void PimaxHandler::_damaged() { if (_entry) _entry->invalid(); }

void PimaxHandler::_load_pedestals()
{
  const DescImage& d = _pentry->desc();

  _offset    = make_ndarray<unsigned>(d.nbinsy(),d.nbinsx());
  for(unsigned* a = _offset.begin(); a!=_offset.end(); *a++ = offset) ;

  EntryImage* p = _pentry;
  if (!FrameCalib::load_pedestals(p,offset)) {
    ndarray<unsigned,2> pa = p->content();
    for(unsigned* a=pa.begin(); a!=pa.end(); *a++=offset) ;
  }
}
