#include "JungfrauHandler.hh"

#include "ami/event/GainSwitchCalib.hh"
#include "ami/event/JungfrauAlignment.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const unsigned offset=1<<16;

typedef Pds::Jungfrau::ConfigV1 CfgJfV1;
typedef Pds::Jungfrau::ConfigV2 CfgJfV2;
typedef Pds::Jungfrau::ConfigV3 CfgJfV3;
typedef Pds::Jungfrau::ElementV1 ElemJfV1;
typedef Pds::Jungfrau::ElementV2 ElemJfV2;

namespace Ami {
  namespace Jungfrau {
    class ConfigCache {
    public:
      static ConfigCache* instance(Pds::TypeId config_type,
                                   const void* config_payload);
      virtual ~ConfigCache() {}
    public:
      virtual unsigned numberOfModules         () const = 0;
      virtual unsigned numberOfRowsPerModule   () const = 0;
      virtual unsigned numberOfColumnsPerModule() const = 0;

      virtual unsigned numberOfGains() const;
      virtual unsigned gainBitMask  () const;
      virtual unsigned dataBitMask  () const;

      virtual Ami::DescImage descImage  (const Pds::DetInfo&, bool) const;
      virtual void dump() const = 0;

      template<class Elem>
      ndarray<const uint16_t,3> frame(const void* payload) const;
    protected:
      virtual ndarray<const uint16_t,3> _frame(const ElemJfV1* elem) const = 0;
      virtual ndarray<const uint16_t,3> _frame(const ElemJfV2* elem) const = 0;

      static const unsigned NBITS = 16;
    private:
      template<class Cfg>
      static ConfigCache* _create(const void* config_payload);
    };

    template<class Cfg>
    class VersionCache : public ConfigCache {
    public:
      VersionCache(const Cfg* config) :
        _config(new Cfg(*config)) {}
      ~VersionCache() { delete _config; }
    public:
      unsigned numberOfModules         () const { return _config->numberOfModules(); }
      unsigned numberOfRowsPerModule   () const { return _config->numberOfRowsPerModule(); }
      unsigned numberOfColumnsPerModule() const { return _config->numberOfColumnsPerModule(); }

      ndarray<const uint16_t,3> _frame(const ElemJfV1* elem) const { return elem->frame(*_config); }
      ndarray<const uint16_t,3> _frame(const ElemJfV2* elem) const { return elem->frame(*_config); }

      void dump() const {};
    private:
      Cfg* _config;
    };
  }
}

using namespace Ami;

Ami::DescImage Jungfrau::ConfigCache::descImage(const Pds::DetInfo& det, bool full_resolution) const
{
  // Create alignment object
  Alignment::Jungfrau align(det,
                            numberOfModules(),
                            numberOfRowsPerModule(),
                            numberOfColumnsPerModule());

  // Get the image width and height from the alignment object
  unsigned width  = align.width();
  unsigned height = align.height();
  unsigned pixels = (width > height) ? width : height;
  unsigned ppb    = full_resolution ? 1 : (pixels-1)/512 + 1;

  width  = (width +ppb-1)/ppb;
  height = (height+ppb-1)/ppb;

  DescImage desc(det, (unsigned)0, ChannelID::name(det),
                 width, height, ppb, ppb);
  // add the frames to the DescImage
  align.add_frames(desc, ppb, ppb);

  return desc;
}

template<class Elem>
ndarray<const uint16_t,3> Jungfrau::ConfigCache::frame(const void* payload) const
{
  const Elem* elem = reinterpret_cast<const Elem*>(payload);
  return _frame(elem);
}

Jungfrau::ConfigCache* Jungfrau::ConfigCache::instance(Pds::TypeId config_type,
                                                       const void* config_payload)
{
  Jungfrau::ConfigCache* cache = NULL;
  switch(config_type.id()) {
    case Pds::TypeId::Id_JungfrauConfig:
      switch(config_type.version()) {
        case 1:
          cache = _create<CfgJfV1>(config_payload);
          break;
        case 2:
          cache = _create<CfgJfV2>(config_payload);
          break;
        case 3:
          cache = _create<CfgJfV3>(config_payload);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  return cache;
}

template<class C>
Jungfrau::ConfigCache* Jungfrau::ConfigCache::_create(const void*  config_payload)
{
  const C* cfg = reinterpret_cast<const C*>(config_payload);
  return new Jungfrau::VersionCache<C>(cfg);
}

unsigned Jungfrau::ConfigCache::numberOfGains() const
{
  return 3;
}

unsigned Jungfrau::ConfigCache::gainBitMask() const
{
  unsigned nbits = 0;
  unsigned ngains = numberOfGains();
  while (ngains > 0) {
    nbits++;
    ngains >>= 1;
  }
  return (1<<NBITS) - (1<<(NBITS-nbits));
}

unsigned Jungfrau::ConfigCache::dataBitMask() const
{
  return ((1<<NBITS) - 1) - gainBitMask();
}

using namespace Pds;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_JungfrauElement);
  return types;
}

JungfrauHandler::JungfrauHandler(const Pds::DetInfo& info, FeatureCache& cache) : 
  EventHandler(info, data_type_list(), Pds::TypeId::Id_JungfrauConfig),
  _config_cache(0),
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
  if (_config_cache) delete _config_cache;
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

void JungfrauHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());

  // cache the Jungfrau configuration
  if (_config_cache) delete _config_cache;
  _config_cache = Jungfrau::ConfigCache::instance(tid,payload);
  _config_cache->dump();
  
  //  Construct Jungfrau modules and place them in the large rectangular frame
  DescImage desc = _config_cache->descImage(det, _full_resolution());
  _entry = new EntryImage(desc);
  _entry->invalid();

  _load_pedestals();
  _load_offsets();
  _load_gains();
}   

void JungfrauHandler::_calibrate(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  // cache the calib cylce changes to the Jungfrau configuration
  if (_config_cache) delete _config_cache;
  _config_cache = Jungfrau::ConfigCache::instance(tid,payload);
  _config_cache->dump();
}

void JungfrauHandler::_event(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.id() == Pds::TypeId::Id_JungfrauElement)
  {
    if (!_entry) return;

    _entry->reset();

    const DescImage& desc = _entry->desc();
    unsigned o = desc.options();
    if (_options != o) {
      printf("JungfrauHandler options %x -> %x\n",_options,o);
      _options = desc.options();
    }
    unsigned gain_bits = _config_cache->gainBitMask();
    unsigned data_bits = _config_cache->dataBitMask();
    unsigned num_gains = _config_cache->numberOfGains();
    unsigned modules   = _config_cache->numberOfModules();
    unsigned columns   = _config_cache->numberOfColumnsPerModule();
    unsigned rows      = _config_cache->numberOfRowsPerModule();

    if (desc.options() & GainSwitchCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( desc.options()&~GainSwitchCalib::option_reload_pedestal() );
    }

    int ppbin = _entry->desc().ppxbin();
    ndarray<const uint16_t, 3> src;
    switch(type.version()) {
      case 1:
        src = _config_cache->frame<Pds::Jungfrau::ElementV1>(payload);
        break;
      case 2:
        src = _config_cache->frame<Pds::Jungfrau::ElementV2>(payload);
        break;
      default:
        return;
    }

    for(unsigned i=0; i<modules; i++) {
      bool fk=_entry->desc().frame(i).flipx;
      bool fj=_entry->desc().frame(i).flipy;
      ndarray<unsigned,2> dst(_entry->contents(i));
      for(unsigned j=0, jn=rows-1; j<rows; j++, jn--) {
        for(unsigned k=0, kn=columns-1; k<columns; k++, kn--) {
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
            dst((fj ? jn : j)/ppbin, (fk ? kn : k)/ppbin) += (unsigned) calib_val;
            break;
          case D90:
            dst((fk ? k : kn)/ppbin, (fj ? jn : j)/ppbin) += (unsigned) calib_val;
            break;
          case D180:
            dst((fj ? j : jn)/ppbin, (fk ? k : kn)/ppbin) += (unsigned) calib_val;
            break;
          case D270:
            dst((fk ? kn : k)/ppbin, (fj ? j : jn)/ppbin) += (unsigned) calib_val;
            break;
          default:
            break;
          }
        }
      }
    }

    _entry->info(double(offset*ppbin*ppbin),EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void JungfrauHandler::_damaged() { if (_entry) _entry->invalid(); }

void JungfrauHandler::_load_pedestals()
{
  _pedestal = _load_calib("None", "pedestals", 0.0);
}

void JungfrauHandler::_load_offsets()
{
  _offset = _load_calib("None", "pixel_offset", 0.0);
}

void JungfrauHandler::_load_gains()
{
  _gain_cor = _load_calib("None", "pixel_gain", 1.0);
}

ndarray<double,4> JungfrauHandler::_load_calib(const char* online,
                                               const char* offline,
                                               double default_val,
                                               bool* used_default) const
{
  return GainSwitchCalib::load_multi_array(_entry->desc().info(),
                                           _config_cache->numberOfGains(),
                                           _config_cache->numberOfModules(),
                                           _config_cache->numberOfRowsPerModule(),
                                           _config_cache->numberOfColumnsPerModule(),
                                           default_val,
                                           used_default,
                                           online,
                                           offline);
}
