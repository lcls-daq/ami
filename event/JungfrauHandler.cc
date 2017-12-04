#include "JungfrauHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

static const unsigned num_gains = 3;
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
    CASE_VSN(3);
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
    CASE_VSN(3);
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
    CASE_VSN(3);
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
    CASE_VSN(3);
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
  _offset       (make_ndarray<double>(0U,0,0,0)),
  _pedestal     (make_ndarray<double>(0U,0,0,0)),
  _gain_cor     (make_ndarray<double>(0U,0,0,0)),
  _options      (0)
{
}

JungfrauHandler::~JungfrauHandler()
{
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
}

void JungfrauHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
  { const Xtc* tc = reinterpret_cast<const Xtc*>(payload)-1;
    _configtc = reinterpret_cast<Xtc*>(new char[tc->extent]);
    memcpy(_configtc, tc, tc->extent); }
  
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  unsigned columns = width (_configtc);
  unsigned rows    = height(_configtc);
  unsigned pixels  = (columns > rows) ? columns : rows;
  unsigned ppb     = _full_resolution() ? 1 : (pixels-1)/640 + 1;


  columns = (columns+ppb-1)/ppb;
  rows    = (rows   +ppb-1)/ppb;

  DescImage desc(det, (unsigned)0, ChannelID::name(det),
                 columns, rows, ppb, ppb);
  _entry = new EntryImage(desc);
  _entry->invalid();

  _load_pedestals(num_modules(_configtc), num_rows(_configtc), num_columns(_configtc));
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
    
    unsigned modules = num_modules(_configtc);
    unsigned columns = num_columns(_configtc);
    unsigned rows    = num_rows(_configtc);

    if (desc.options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals(modules, rows, columns);
      _entry->desc().options( desc.options()&~FrameCalib::option_reload_pedestal() );
    }

    int ppbin = _entry->desc().ppxbin();
    memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
    ndarray<const uint16_t, 3> d;
    switch(type.version()) {
      case 1:
        d = array<Pds::Jungfrau::ElementV1>(_configtc, payload);
        break;
      case 2:
        d = array<Pds::Jungfrau::ElementV2>(_configtc, payload);
        break;
      default:
        return;
    }

    for(unsigned i=0; i<modules; i++) {
      for(unsigned j=0,j_flip=rows-1; j<rows; j++,j_flip--) {
        for(unsigned k=0; k<columns; k++) {
          unsigned gain_val = (d(i,j,k) & gain_bits) >> 14;
          // The gain index to use is the highest of the set bits
          unsigned gain_idx = 0;
          for (unsigned n = 0; n<num_gains-1; n++) {
            if ((1<<n) & gain_val) gain_idx = n+1;
          }
          unsigned pixel_val = d(i,j,k) & data_bits;
          unsigned row_bin = (rows * i) + (rows - 1 - j);
          double calib_val = 0.0;
          if (!(desc.options()&FrameCalib::option_no_pedestal())) {
            if (desc.options()&FrameCalib::option_correct_gain()) {
              calib_val = (pixel_val - _pedestal(gain_idx,i,j,k) - _offset(gain_idx,i,j,k))/_gain_cor(gain_idx,i,j,k) + offset;
            } else {
              calib_val = pixel_val - _pedestal(gain_idx,i,j,k) - _offset(gain_idx,i,j,k) + offset;
            }
          } else {
            calib_val = (double) (pixel_val + offset);
          }
          if (calib_val < 0.0) calib_val = 0.0; // mask the problem negative pixels
          _entry->addcontent((unsigned) calib_val, k/ppbin, row_bin/ppbin);
        }
      }
    }

    _entry->info(double(offset*ppbin*ppbin),EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void JungfrauHandler::_damaged() { if (_entry) _entry->invalid(); }

void JungfrauHandler::_load_pedestals(unsigned modules, unsigned rows, unsigned columns)
{
  const DescImage& d = _entry->desc();
  _offset = FrameCalib::load_multi_array(d.info(), num_gains, modules, rows, columns, 0.0, "None", "pixel_offset");
  _pedestal = FrameCalib::load_multi_array(d.info(), num_gains, modules, rows, columns, 0.0, "None", "pedestals");
  _gain_cor = FrameCalib::load_multi_array(d.info(), num_gains, modules, rows, columns, 1.0, "None", "pixel_gain");
}
