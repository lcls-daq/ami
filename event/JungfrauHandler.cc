#include "JungfrauHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

static const unsigned offset=1<<16;
static const unsigned gain_bits = 3<<14;
static const unsigned data_bits = ((1<<16) - 1) - gain_bits;

static inline unsigned num_modules(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                             \
  { const Pds::Jungfrau::ConfigV##v& c =                                   \
      *reinterpret_cast<const Pds::Jungfrau::ConfigV##v*>(tc->payload());  \
      return c.numberOfModules(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned num_rows(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                             \
  { const Pds::Jungfrau::ConfigV##v& c =                                   \
      *reinterpret_cast<const Pds::Jungfrau::ConfigV##v*>(tc->payload());  \
      return c.numberOfRowsPerModule(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned num_columns(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                             \
  { const Pds::Jungfrau::ConfigV##v& c =                                   \
      *reinterpret_cast<const Pds::Jungfrau::ConfigV##v*>(tc->payload());  \
      return c.numberOfColumnsPerModule(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned height(const Xtc* tc)
{
  /* For now all the modules will be displayed in memory order. Need a geometry... */
  return num_rows(tc) * num_modules(tc);
}

static inline unsigned width(const Xtc* tc)
{
  return num_columns(tc);
}

template <class Element>
static inline ndarray<const uint16_t,3> array(const Xtc* tc,
                                              const void* f)
{
  #define CASE_VSN(v) case v:                                           \
  { const Pds::Jungfrau::ConfigV##v& c =                                   \
      *reinterpret_cast<const Pds::Jungfrau::ConfigV##v*>(tc->payload());  \
    return reinterpret_cast<const Element*>(f)->frame(c); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return ndarray<const uint16_t,3>();
}

using namespace Ami;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_JungfrauElement);
  return types;
}

JungfrauHandler::JungfrauHandler(const Pds::DetInfo& info, FeatureCache& cache) : 
  EventHandler(info, data_type_list(), Pds::TypeId::Id_JungfrauConfig),
  _configtc(0),
  _cache(cache),
  _entry        (0),
  _pentry       (0),
  _offset       (make_ndarray<unsigned>(1,1)),
  _options      (0)
{
}

JungfrauHandler::~JungfrauHandler()
{
  if (_pentry)
    delete _pentry;
  if (_configtc) delete[] reinterpret_cast<char*>(_configtc);
}

unsigned JungfrauHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* JungfrauHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void JungfrauHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void JungfrauHandler::reset() 
{
  _entry = 0; 
  if (_pentry) { 
    delete _pentry; 
    _pentry=0; 
  }
}

void JungfrauHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
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

  /*
   * Setup temperature variable
   */
}

void JungfrauHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) {}

void JungfrauHandler::_event(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.id() == Pds::TypeId::Id_JungfrauElement)
  {
    if (!_entry) return;

    const DescImage& desc = _entry->desc();
    unsigned o = desc.options();
    if (_options != o) {
      printf("JungfrauHandler options %x -> %x\n",_options,o);
      _options = desc.options();
    }
    
    if (desc.options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( desc.options()&~FrameCalib::option_reload_pedestal() );
    }

    const ndarray<const unsigned,2>& pa =
      desc.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pentry->content();

    unsigned columns = num_columns(_configtc);
    unsigned rows    = num_rows(_configtc);
    int ppbin = _entry->desc().ppxbin();
    memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
    const uint16_t* d = 0;
    switch(type.version()) {
      case 1:
        d = reinterpret_cast<const uint16_t*>(array<Pds::Jungfrau::ElementV1>(_configtc, payload).data());
        break;
      case 2:
        d = reinterpret_cast<const uint16_t*>(array<Pds::Jungfrau::ElementV2>(_configtc, payload).data());
        break;
      default:
        return;
    }
    for(unsigned j=0; j<height(_configtc); j++) {
      const unsigned* p = &pa[j][0];
      for(unsigned k=0; k<width(_configtc); k++, p++)
        _entry->addcontent((d[rows*columns*(j/rows) + columns*(rows - (j%rows) - 1) + k] & data_bits) + *p, k/ppbin, j/ppbin);
    }

    //  _entry->info(f.offset()*ppbx*ppby,EntryImage::Pedestal);
    _entry->info(double(offset*ppbin*ppbin),EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void JungfrauHandler::_damaged() { if (_entry) _entry->invalid(); }

void JungfrauHandler::_load_pedestals()
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
