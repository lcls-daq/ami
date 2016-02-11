#include "DualAndorHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const unsigned offset=1<<16;
static const unsigned sensor_gap=808;

static inline unsigned height(const Pds::Andor3d::ConfigV1& c)
{
  return c.numPixelsY();
}

static unsigned width(const Pds::Andor3d::ConfigV1& c)
{
  return c.numPixelsX();
}

static unsigned sensors(const Pds::Andor3d::ConfigV1& c)
{
  return c.numSensors();
}

static unsigned gap_size(const Pds::Andor3d::ConfigV1& c)
{
  return (sensor_gap + c.binX() - 1) / c.binX();
}

using namespace Ami;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_Andor3dFrame);
  return types;
}

DualAndorHandler::DualAndorHandler(const Pds::DetInfo& info, FeatureCache& cache) : 
  EventHandler(info, data_type_list(), Pds::TypeId::Id_Andor3dConfig),
  _cache(cache),
  _entry        (0),
  _pentry       (0),
  _offset       (make_ndarray<unsigned>(1,1)),
  _options      (0)
{
}

DualAndorHandler::~DualAndorHandler()
{
  if (_pentry)
    delete _pentry;
}

unsigned DualAndorHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* DualAndorHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void DualAndorHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void DualAndorHandler::reset() 
{
  _entry = 0; 
  if (_pentry) { 
    delete _pentry; 
    _pentry=0; 
  }
}

void DualAndorHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{  
  if (type.version() == Pds::Andor3d::ConfigV1::Version)
    _config = *reinterpret_cast<const Pds::Andor3d::ConfigV1*>(payload);
  else
    printf("DualAndorHandler::_configure(): Unsupported Andor Version %d\n", type.version());    

  unsigned nsensor = sensors(_config);
  unsigned scols   = width (_config);
  unsigned srows   = height(_config);
  unsigned columns = scols * nsensor;
  unsigned rows    = srows;
  unsigned gap     = gap_size(_config);
  unsigned pixels  = (columns > rows) ? columns : rows;
  unsigned ppb     = _full_resolution() ? 1 : (pixels-1)/1024 + 1;
  // check that binning is an even divison 
  if (columns % ppb || rows % ppb)
    ppb = 1;
  columns = (columns+ppb-1)/ppb;
  rows    = (rows   +ppb-1)/ppb;
  scols   = (scols  +ppb-1)/ppb;
  srows   = (srows  +ppb-1)/ppb;
  gap     = (gap    +ppb-1)/ppb;

  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  { DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns + gap, rows, ppb, ppb);
    //desc.add_frame(columns - scols,       0, gap,   srows);
    desc.add_frame(0,                     0, scols, srows);
    desc.add_frame(columns + gap - scols, 0, scols, srows);
    _entry  = new EntryImage(desc); }

  { DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns*ppb, rows*ppb, 1, 1);
    desc.add_frame(0,                     0, scols*ppb, srows*ppb);
    desc.add_frame((columns - scols)*ppb, 0, scols*ppb, srows*ppb);
    _pentry = new EntryImage(desc);
    _pentry->invalid(); }

  _load_pedestals();

  /*
   * Setup temperature variable
   */
  _iCacheIndexTemperature = make_ndarray<unsigned>(nsensor);
  char sTemperatureVar[64];
  for(unsigned i=0;i<nsensor;i++) {
    sprintf(sTemperatureVar, "DualAndor-%d-T%d", det.devId(), i);
    _iCacheIndexTemperature[i] = _cache.add(sTemperatureVar);
  }
}

void DualAndorHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) {}

void DualAndorHandler::_event(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.id() == Pds::TypeId::Id_Andor3dFrame)
  {
    const Pds::Andor3d::FrameV1& f = *reinterpret_cast<const Pds::Andor3d::FrameV1*>(payload);
    if (!_entry) return;

    const DescImage& desc = _entry->desc();
    unsigned o = desc.options();
    if (_options != o) {
      printf("DualAndorHandler options %x -> %x\n",_options,o);
      _options = desc.options();
    }
    
    if (desc.options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( desc.options()&~FrameCalib::option_reload_pedestal() );
    }

    const ndarray<const unsigned,2>& pa =
      desc.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pentry->content();

    unsigned ppbin = _entry->desc().ppxbin();
    memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
    const uint16_t* d = reinterpret_cast<const uint16_t*>(f.data(_config).data());
    const SubFrame& frame0 = _entry->desc().frame(0);
    const SubFrame& frame1 = _entry->desc().frame(1);
    const unsigned gapbin = frame1.x - (frame0.x + frame0.nx);

    for(unsigned s=0; s<_entry->desc().nframes(); s++) {
      const SubFrame& pframe = _pentry->desc().frame(s);
      for(unsigned j=pframe.y; j<(pframe.y+pframe.ny); j++) {
        const unsigned* p = &pa[j][pframe.x];
        for(unsigned k=pframe.x; k<(pframe.x+pframe.nx); k++, d++, p++)
          _entry->addcontent(*d + *p, k/ppbin+s*gapbin, j/ppbin);
      }
    }

    //  _entry->info(f.offset()*ppbx*ppby,EntryImage::Pedestal);
    _entry->info(double(offset*ppbin*ppbin),EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);
    
    ndarray<const float, 1> temps = f.temperature(_config);
    for(unsigned s=0; s<sensors(_config); s++)
      _cache.cache(_iCacheIndexTemperature[s], temps[s]);
  }
}

void DualAndorHandler::_damaged() { if (_entry) _entry->invalid(); }

void DualAndorHandler::_load_pedestals()
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
