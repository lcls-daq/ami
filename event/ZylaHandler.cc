#include "ZylaHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

static const unsigned offset=1<<16;

static inline unsigned height(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                           \
  { const Pds::Zyla::ConfigV##v& c =                                  \
      *reinterpret_cast<const Pds::Zyla::ConfigV##v*>(tc->payload()); \
      return c.height()/c.binY(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned width(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                           \
  { const Pds::Zyla::ConfigV##v& c =                                  \
      *reinterpret_cast<const Pds::Zyla::ConfigV##v*>(tc->payload()); \
      return c.width()/c.binX(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline ndarray<const uint16_t,2> array(const Xtc* tc,
                                              const Pds::Zyla::FrameV1& f)
{
  #define CASE_VSN(v) case v:                                           \
  { const Pds::Zyla::ConfigV##v& c =                                    \
      *reinterpret_cast<const Pds::Zyla::ConfigV##v*>(tc->payload());   \
    return f.data(c); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return ndarray<const uint16_t,2>();
}

using namespace Ami;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_ZylaFrame);
  return types;
}

ZylaHandler::ZylaHandler(const Pds::DetInfo& info, FeatureCache& cache) : 
  EventHandler(info, data_type_list(), Pds::TypeId::Id_ZylaConfig),
  _configtc(0),
  _cache(cache),
  _entry        (0),
  _pentry       (0),
  _offset       (make_ndarray<unsigned>(1,1)),
  _options      (0)
{
}

//ZylaHandler::ZylaHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
//  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_ZylaConfig),
//  _entry(entry ? new EntryImage(entry->desc()) : 0)
//{
//}

ZylaHandler::~ZylaHandler()
{
  if (_pentry)
    delete _pentry;
  if (_configtc) delete[] reinterpret_cast<char*>(_configtc);
}

unsigned ZylaHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* ZylaHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void ZylaHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void ZylaHandler::reset() 
{
  _entry = 0; 
  if (_pentry) { 
    delete _pentry; 
    _pentry=0; 
  }
}

void ZylaHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
  { const Xtc* tc = reinterpret_cast<const Xtc*>(payload)-1;
    _configtc = reinterpret_cast<Xtc*>(new char[tc->extent]);
    memcpy(_configtc, tc, tc->extent); }
  
  unsigned columns = width (_configtc);
  unsigned rows    = height(_configtc);
  unsigned pixels  = (columns > rows) ? columns : rows;
  unsigned ppb     = _full_resolution() ? 1 : (pixels-1)/640 + 1;
  columns = (columns+ppb-1)/ppb;
  rows    = (rows   +ppb-1)/ppb;
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  { DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns, rows, ppb, ppb);
    _entry  = new EntryImage(desc); }
    
  { DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns*ppb, rows*ppb, 1, 1);
    _pentry = new EntryImage(desc);
    _pentry->invalid(); }

  _load_pedestals();
}

void ZylaHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) {}

void ZylaHandler::_event(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.id() == Pds::TypeId::Id_ZylaFrame)
  {
    const Pds::Zyla::FrameV1& f = *reinterpret_cast<const Pds::Zyla::FrameV1*>(payload);
    if (!_entry) return;

    const DescImage& desc = _entry->desc();
    unsigned o = desc.options();
    if (_options != o) {
      printf("ZylaHandler options %x -> %x\n",_options,o);
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
    const uint16_t* d = reinterpret_cast<const uint16_t*>(array(_configtc, f).data());
    for(unsigned j=0; j<height(_configtc); j++) {
      const unsigned* p = &pa[j][0];
      for(unsigned k=0; k<width(_configtc); k++, d++, p++)
        _entry->addcontent(*d + *p, k/ppbin, j/ppbin);
    }

    //  _entry->info(f.offset()*ppbx*ppby,EntryImage::Pedestal);
    _entry->info(double(offset*ppbin*ppbin),EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void ZylaHandler::_damaged() { if (_entry) _entry->invalid(); }

void ZylaHandler::_load_pedestals()
{
  const DescImage& d = _pentry->desc();

  _offset    = make_ndarray<unsigned>(d.nbinsy(),d.nbinsx());
  for(unsigned* a = _offset.begin(); a!=_offset.end(); *a++ = offset) ;

  EntryImage* p = _pentry;
  if (!FrameCalib::load_pedestals_all(p,offset)) {
    ndarray<unsigned,2> pa = p->content();
    for(unsigned* a=pa.begin(); a!=pa.end(); *a++=offset) ;
  }
}
