#include "JungfrauHandler.hh"

#include "ami/event/GainSwitchCalib.hh"
#include "ami/event/JungfrauAlignment.hh"
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
static const double quasi_adu_factor = 40.0;

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
  _options      (0),
  _do_norm      (false)
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
  
  _load_geometry(num_modules(_configtc), num_rows(_configtc), num_columns(_configtc));

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
    double   norm    = 1.0;

    if (desc.options() & GainSwitchCalib::option_reload_pedestal()) {
      _load_pedestals(modules, rows, columns);
      _entry->desc().options( desc.options()&~GainSwitchCalib::option_reload_pedestal() );
    }

    // Set the normalization if gain correcting
    unsigned norm_mask = GainSwitchCalib::normalization_option_mask();
    if (!(desc.options()&GainSwitchCalib::option_no_pedestal())) {
      if (_do_norm && ((desc.options()&norm_mask) == norm_mask)) {
        norm = quasi_adu_factor;
      }
    }

    int ppbin = _entry->desc().ppxbin();
    memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
    ndarray<const uint16_t, 3> src;
    switch(type.version()) {
      case 1:
        src = array<Pds::Jungfrau::ElementV1>(_configtc, payload);
        break;
      case 2:
        src = array<Pds::Jungfrau::ElementV2>(_configtc, payload);
        break;
      default:
        return;
    }

    for(unsigned i=0; i<modules; i++) {
      ndarray<unsigned,2> dst(_entry->contents(i));
      for(unsigned j=0; j<rows; j++) {
        for(unsigned k=0; k<columns; k++) {
          unsigned gain_val = (src(i,j,k) & gain_bits) >> 14;
          // The gain index to use is the highest of the set bits
          unsigned gain_idx = 0;
          for (unsigned n = 0; n<num_gains-1; n++) {
            if ((1<<n) & gain_val) gain_idx = n+1;
          }
          unsigned pixel_val = src(i,j,k) & data_bits;
          double calib_val = 0.0;
          if (!(desc.options()&GainSwitchCalib::option_no_pedestal())) {
            if (desc.options()&GainSwitchCalib::option_correct_gain()) {
              calib_val = (pixel_val - _pedestal(gain_idx,i,j,k) - _offset(gain_idx,i,j,k))/_gain_cor(gain_idx,i,j,k) + offset;
            } else {
              calib_val = pixel_val - _pedestal(gain_idx,i,j,k) - _offset(gain_idx,i,j,k) + offset;
            }
          } else {
            calib_val = (double) (pixel_val + offset);
          }
          if (calib_val < 0.0) calib_val = 0.0; // mask the problem negative pixels
          switch(_entry->desc().frame(i).r) {
          case D0:
            dst(k/ppbin, j/ppbin) += (unsigned) calib_val;
            break;
          case D90:
            dst((rows -1 -j)/ppbin, k/ppbin) += (unsigned) calib_val;
            break;
          case D180:
            dst((columns -1 -k)/ppbin, (rows -1 -j)/ppbin) += (unsigned) calib_val;
            break;
          case D270:
            dst(j/ppbin, (columns -1 -k)/ppbin) += (unsigned) calib_val;
            break;
          default:
            break;
          }
        }
      }
    }

    _entry->info(double(offset*ppbin*ppbin),EntryImage::Pedestal);
    _entry->info(norm,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void JungfrauHandler::_damaged() { if (_entry) _entry->invalid(); }

void JungfrauHandler::_load_geometry(unsigned modules, unsigned rows, unsigned columns)
{
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  Alignment::Jungfrau align(det, modules, rows, columns);

  unsigned width  = align.width();
  unsigned height = align.height();
  unsigned pixels = (width > height) ? width : height;
  unsigned ppb    = _full_resolution() ? 1 : (pixels-1)/512 + 1;

  width  = (width +ppb-1)/ppb;
  height = (height+ppb-1)/ppb;

  DescImage desc(det, (unsigned)0, ChannelID::name(det),
                 width, height, ppb, ppb);
  // add the frames to the DescImage
  align.add_frames(desc, ppb, ppb);

  _entry = new EntryImage(desc);
  _entry->invalid();
}

void JungfrauHandler::_load_pedestals(unsigned modules, unsigned rows, unsigned columns)
{
  bool used_default_gain = false;
  const DescImage& d = _entry->desc();
  _offset = GainSwitchCalib::load_multi_array(d.info(), num_gains, modules, rows, columns, 0.0, NULL, "None", "pixel_offset");
  _pedestal = GainSwitchCalib::load_multi_array(d.info(), num_gains, modules, rows, columns, 0.0, NULL, "None", "pedestals");
  _gain_cor = GainSwitchCalib::load_multi_array(d.info(), num_gains, modules, rows, columns, 1.0, &used_default_gain, "None", "pixel_gain");
  if (!used_default_gain) {
    _do_norm = true;
    for(double* val = _gain_cor.begin(); val!=_gain_cor.end(); val++) (*val) /= quasi_adu_factor;
  }
}
