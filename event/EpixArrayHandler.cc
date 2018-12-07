#include "EpixArrayHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/event/GainSwitchCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/xtc/ClockTime.hh"

#include <string.h>
#include <sstream>

//#define DBUG

typedef Pds::Epix::Config10ka2MV1   Cfg10ka2M;
typedef Pds::Epix::Config10kaQuadV1 Cfg10kaQuad;
typedef Pds::Epix::Config10ka       Cfg10ka;

static const unsigned      wE = Cfg10ka::_numberOfPixelsPerAsicRow*Cfg10ka::_numberOfAsicsPerRow;
static const unsigned      hE = Cfg10ka::_numberOfRowsPerAsic     *Cfg10ka::_numberOfAsicsPerColumn;
//  Three margin parameters
static const unsigned      gM = 2;
static const unsigned      eM = 80;  // "edge" margin
static const unsigned      hM = 4;   // 1/2 "horizontal" margin between ASICs
static const unsigned      vM = 12;  // 1/2 "vertical" margin between ASICs
static const unsigned      asicMap[] = { 2, 1, 3, 0 };

using Ami::EventHandlerF;
using Ami::FeatureCache;
using Ami::Entry;
using Ami::EntryRef;
using Ami::EntryWaveform;

namespace EpixArray {

  class EnvData {
  public:
    virtual ~EnvData() {}
  public:
    virtual void     addFeatures(const char* name) = 0;
    virtual void     rename     (const char* name) = 0;
    virtual void     fill       (FeatureCache&, ndarray<const uint32_t,3> env) = 0;
  };

  class EnvDataQuad : public EnvData {
  public:
    EnvDataQuad(Ami::EventHandlerF&);
    ~EnvDataQuad() {}
  public:
    void     addFeatures(const char* name);
    void     rename     (const char* name);
    void     fill       (FeatureCache&, ndarray<const uint32_t,3> env);
    void     _fill      (FeatureCache&, ndarray<const uint32_t,2> env);
  private:
    EventHandlerF& _handler;
    ndarray<int,1> _index;
  };

  class EnvData2M : public EnvData {
  public:
    EnvData2M(EventHandlerF&);
    ~EnvData2M();
  public:
    void     addFeatures(const char* name);
    void     rename     (const char* name);
    void     fill       (FeatureCache&, ndarray<const uint32_t,3> env);
  private:
    std::vector<std::string > _name;
    std::vector<EnvDataQuad*> _quad;
  };

  class CalData {
  public:
    CalData(unsigned n) : _name(n), _ref(0), _elem(n) {}
    virtual ~CalData();
    void     config     (const Pds::DetInfo&, unsigned, unsigned);
    void     rename     (const char*   name);
    void     reset      ();
    void     fill       (ndarray<const uint16_t,3>,
                         const Pds::ClockTime&);
    unsigned nentries   () const { return _ref ? 1 : 0; }
    Entry*   entry      (unsigned) const { return _ref; }
  private:
    virtual void _rename(const char*) = 0;
  protected:
    std::vector< std::string >  _name;
    EntryRef*                   _ref;
    std::vector<EntryWaveform*> _elem;
  };
    
  class CalDataQuad : public CalData {
  public:
    CalDataQuad();
  private:
    void _rename(const char*) {}
  };

  class CalData2M : public CalData {
  public:
    CalData2M();
  private:
    void _rename(const char*) {}
  };
    
  class ConfigCache {
  public:
    static ConfigCache* instance(Pds::TypeId        config_type,
                                 const void*        config_payload,
                                 EventHandlerF&     handler);
    virtual ~ConfigCache() {}
  public:
    virtual unsigned numberOfElements () const = 0;
    virtual unsigned numberOfAsics    () const = 0;
    virtual unsigned numberOfColumns  () const = 0;
    virtual unsigned numberOfRows     () const = 0;
    virtual unsigned numberOfRowsCal  () const = 0;
    virtual unsigned numberOfGainModes() const = 0;
    virtual unsigned numberOfFixedGainModes() const = 0;

    virtual Ami::DescImage descImage  (const Pds::DetInfo&) const = 0;
    virtual EnvData*       envData    () const = 0;
    virtual CalData*       calData    () const = 0;
    virtual ndarray<const uint16_t,3> pixelGainConfig() const = 0;
    virtual ndarray<const uint16_t,3> frame(Pds::TypeId, const void*, FeatureCache&, const Pds::ClockTime&) const = 0;

    virtual void dump() const = 0;
  protected:
    enum { AML=0, FL=8, FM=12, AHL=16, FL_ALT=24, FH=28 };
  };

  class Epix10ka2MCache : public ConfigCache {
  public:
    Epix10ka2MCache(const Cfg10ka2M*    config,
                    Ami::EventHandlerF& handler) :
      _config (new Cfg10ka2M(*config)), 
      _envData(new EnvData2M(handler)),
      _calData(new CalData2M) {}
    ~Epix10ka2MCache() { delete _config; delete _envData; }
  public:
    unsigned numberOfElements () const { return Cfg10ka2M::_numberOfElements;   } //16
    unsigned numberOfAsics    () const { return Cfg10ka2M::_numberOfElements*4; } //64
    unsigned numberOfColumns  () const { return _config->numberOfColumns(); }
    unsigned numberOfRows     () const { return _config->numberOfRows(); }
    unsigned numberOfRowsCal  () const { return _config->numberOfCalibrationRows(); }
    unsigned numberOfGainModes() const { return 7; }
    unsigned numberOfFixedGainModes() const { return 3; }

    Ami::DescImage descImage  (const Pds::DetInfo&) const;
    EnvData*       envData    () const { return _envData; }
    CalData*       calData    () const { return _calData; }
    ndarray<const uint16_t,3> pixelGainConfig() const;
    ndarray<const uint16_t,3> frame(Pds::TypeId, const void*, FeatureCache&, const Pds::ClockTime&) const;

    void dump() const {}
  private:
    Cfg10ka2M*          _config;
    EnvData2M*          _envData;
    CalData2M*          _calData;
  };

  class Epix10kaQuadCache : public ConfigCache {
  public:
    Epix10kaQuadCache(const Cfg10kaQuad*  config,
                      Ami::EventHandlerF& handler) : 
      _config (new Cfg10kaQuad(*config)),
      _envData(new EnvDataQuad(handler)),
      _calData(new CalDataQuad) {}
    ~Epix10kaQuadCache() { delete _config; delete _envData; }
  public:
    unsigned numberOfElements () const { return Cfg10kaQuad::_numberOfElements;   }
    unsigned numberOfAsics    () const { return Cfg10kaQuad::_numberOfElements*4; }
    unsigned numberOfColumns  () const { return _config->numberOfColumns(); }
    unsigned numberOfRows     () const { return _config->numberOfRows(); }
    unsigned numberOfRowsCal  () const { return _config->numberOfCalibrationRows(); }
    unsigned numberOfGainModes() const { return 7; }
    unsigned numberOfFixedGainModes() const { return 3; }

    Ami::DescImage descImage  (const Pds::DetInfo&) const;
    EnvData*       envData    () const { return _envData; }
    CalData*       calData    () const { return _calData; }
    ndarray<const uint16_t,3> pixelGainConfig() const;
    ndarray<const uint16_t,3> frame(Pds::TypeId, const void*, FeatureCache&, const Pds::ClockTime&) const;

    void dump() const {}
  private:
    Cfg10kaQuad*        _config;
    EnvDataQuad*        _envData;
    CalDataQuad*        _calData;
  };
};

using namespace Ami;

EpixArray::ConfigCache* EpixArray::ConfigCache::instance(Pds::TypeId    config_type,
                                                         const void*    config_payload,
                                                         EventHandlerF& handler)
{
  EpixArray::ConfigCache* cache = 0;
  switch(config_type.id()) {
  case Pds::TypeId::Id_Epix10ka2MConfig:
    switch(config_type.version()) {
    case 1: cache = new EpixArray::Epix10ka2MCache(reinterpret_cast<const Cfg10ka2M*>(config_payload),
                                                   handler);
    default: break;
    } break;
  case Pds::TypeId::Id_Epix10kaQuadConfig:
    switch(config_type.version()) {
    case 1: cache = new EpixArray::Epix10kaQuadCache(reinterpret_cast<const Cfg10kaQuad*>(config_payload),
                                                     handler);
    default: break;
    } break;
  default: break;
  }

  if (!cache)
    printf("%s: No configuration for %s\n", __PRETTY_FUNCTION__, Pds::TypeId::name(config_type.id()));
  else {
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(handler.info());
    const char* detname = Pds::DetInfo::name(det.detector());
    cache->envData()->addFeatures(detname);
    cache->calData()->config     (det, 
                                  cache->numberOfRowsCal(), 
                                  cache->numberOfColumns());
  }

  return cache;
}

ndarray<const uint16_t,3> EpixArray::Epix10ka2MCache::pixelGainConfig() const
{
  const unsigned nE = numberOfElements();
  const unsigned conf_bits = 0x1c;
  ndarray<uint16_t,3> pixelGainConfig = make_ndarray<uint16_t>(nE, hE, wE);
  for(unsigned i=0; i<nE; i++) {
    const Cfg10ka& eC = _config->elemCfg(i);
    ndarray<const uint16_t,2> asicPixelConfig = eC.asicPixelConfigArray();
    for(unsigned j=0; j<hE; j++) {
      for(unsigned k=0; k<wE; k++) {
        uint16_t gain_config = 0;
        uint16_t gain_bits = (asicPixelConfig(j,k) & conf_bits) |
                             (eC.asics(asicMap[(j/Cfg10ka::_numberOfRowsPerAsic)*Cfg10ka::_numberOfAsicsPerRow +
                                               k/Cfg10ka::_numberOfPixelsPerAsicRow]).trbit() << 4);
        switch(gain_bits) {
          case FH:
            gain_config = 0;
            break;
          case FM:
            gain_config = 1;
            break;
          case FL:
          case FL_ALT:
            gain_config = 2;
            break;
          case AML:
            gain_config = 3;
            break;
          case AHL:
            gain_config = 3;
            break;
          default:
            printf("Epix10ka2MCache::pixelGainConfig unknown gain control bits %x for pixel (%u, %u)\n", gain_bits, j, k);
            gain_config = 0;
            break;
        }
        pixelGainConfig(i,j,k) = gain_config;
      }
    }
  }

  return pixelGainConfig;
}

DescImage EpixArray::Epix10ka2MCache::descImage(const Pds::DetInfo& det) const
{
  //
  //   Hard code the array's geometry for now
  //
  //  (Epix10ka2m)
  //         |
  //  Quad 0 | Quad 1      Quad 2 is rotated  90d clockwise
  //  -------+--------     Quad 3 is rotated 180d clockwise
  //  Quad 3 | Quad 2      Quad 0 is rotated 270d clockwise
  //         |
  //
  //  (Quad 1)
  //         |
  //  Elem 0 | Elem 1
  //  -------+--------     No rotations
  //  Elem 2 | Elem 3
  //         |
  //
  //  (Elem 0)
  //         |
  //  ASIC 0 | ASIC 3
  //  -------+--------     No rotations
  //  ASIC 1 | ASIC 2
  //         |
  //
  //  (Elem 0-3 pixel array)
  //                    row increasing
  //                          ^
  //                          |
  //                          |
  //  column increasing <-- (0,0)
  //
  //                                         eM     vM     hM     hE     wE, eM     vM     hM     hE       wE
  static const SubFrame elem[] = { SubFrame( eM + 1*vM                     ,             3*hM         + 1*wE, hE, wE, D90 ),
                                   SubFrame( eM + 1*vM                     ,             1*hM               , hE, wE, D90 ),
                                   SubFrame( eM + 3*vM        + 1*hE       ,             3*hM         + 1*wE, hE, wE, D90 ),
                                   SubFrame( eM + 3*vM        + 1*hE       ,             1*hM               , hE, wE, D90 ),

                                   SubFrame( eM + 4*vM + 1*hM + 2*hE       , eM + 1*vM                      , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 3*hM + 2*hE + 1*wE, eM + 1*vM                      , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 1*hM + 2*hE       , eM + 3*vM        + 1*hE        , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 3*hM + 2*hE + 1*wE, eM + 3*vM        + 1*hE        , wE, hE, D180 ),

                                   SubFrame(      3*vM + 3*hM + 1*hE + 2*wE, eM + 3*vM + 2*hM + 2*hE        , hE, wE, D270 ),
                                   SubFrame(      3*vM + 3*hM + 1*hE + 2*wE, eM + 3*vM + 4*hM + 2*hE  + 1*wE, hE, wE, D270 ),
                                   SubFrame(      1*vM + 3*hM        + 2*wE, eM + 3*vM + 2*hM + 2*hE        , hE, wE, D270 ),
                                   SubFrame(      1*vM + 3*hM        + 2*wE, eM + 3*vM + 4*hM + 2*hE  + 1*wE, hE, wE, D270 ),

                                   SubFrame(             3*hM        + 1*wE,      3*vM + 3*hM + 1*hE  + 2*wE, wE, hE, D0 ),
                                   SubFrame(             1*hM              ,      3*vM + 3*hM + 1*hE  + 2*wE, wE, hE, D0 ),
                                   SubFrame(             3*hM        + 1*wE,      1*vM + 3*hM         + 2*wE, wE, hE, D0 ),
                                   SubFrame(             1*hM              ,      1*vM + 3*hM         + 2*wE, wE, hE, D0 ) };

      //
  // Determine the bounds of the larger rectangular frame
  //
  unsigned xmin=-1U, xmax=0, ymin=-1U, ymax=0, v;
  for(unsigned i=0; i<numberOfElements(); i++)
    //    if (_config->elemCfg(i).asicMask()) {
    if (1) {
      if ((v=elem[i].x             ) < xmin) xmin = v;
      if ((v=elem[i].x + elem[i].nx) > xmax) xmax = v;
      if ((v=elem[i].y)              < ymin) ymin = v;
      if ((v=elem[i].y + elem[i].ny) > ymax) ymax = v;
    }

  unsigned wImage = xmax - xmin + 2*gM;
  unsigned hImage = ymax - ymin + 2*gM;

  printf("x [%u,%u]  y [%u,%u]  w %u  h %u\n",
         xmin, xmax, ymin, ymax, wImage, hImage);

  //
  // Place each element within the larger rectangular frame
  //
  unsigned ppb=1, dpb=1; // pixels per bin, display ppb
  DescImage desc(det, unsigned(0), ChannelID::name(det),
                 wImage, hImage, ppb, ppb, dpb, dpb);
  for(unsigned i=0; i<numberOfElements(); i++)
    //    if (_config->elemCfg(i).asicMask()) {
    if (1) {
      SubFrame fr(elem[i]);
      fr.x += gM - xmin;
      fr.y += gM - ymin;
      desc.add_frame( fr );
    }
  return desc;
}

ndarray<const uint16_t,3> EpixArray::Epix10ka2MCache::frame(Pds::TypeId tid, 
                                                            const void* payload,
                                                            FeatureCache& cache,
                                                            const Pds::ClockTime& t) const
{
  const Pds::Epix::ArrayV1* array = reinterpret_cast<const Pds::Epix::ArrayV1*>(payload);
  ndarray<const uint16_t,3> a = array->frame(*_config);
  _envData->fill( cache, array->environmentalRows(*_config) );
  _calData->fill( array->calibrationRows(*_config), t );
  return a;
}

ndarray<const uint16_t,3> EpixArray::Epix10kaQuadCache::pixelGainConfig() const {
  const unsigned nE = numberOfElements();
  const unsigned conf_bits = 0x1c;
  ndarray<uint16_t,3> pixelGainConfig = make_ndarray<uint16_t>(nE, hE, wE);
  for(unsigned i=0; i<nE; i++) {
    const Cfg10ka& eC = _config->elemCfg(i);
    ndarray<const uint16_t,2> asicPixelConfig = eC.asicPixelConfigArray();
    for(unsigned j=0; j<hE; j++) {
      for(unsigned k=0; k<wE; k++) {
        uint16_t gain_config = 0;
        uint16_t gain_bits = (asicPixelConfig(j,k) & conf_bits) |
                             (eC.asics(asicMap[(j/Cfg10ka::_numberOfRowsPerAsic)*Cfg10ka::_numberOfAsicsPerRow +
                                               k/Cfg10ka::_numberOfPixelsPerAsicRow]).trbit() << 4);
        switch(gain_bits) {
          case FH:
            gain_config = 0;
            break;
          case FM:
            gain_config = 1;
            break;
          case FL:
          case FL_ALT:
            gain_config = 2;
            break;
          case AHL:
            gain_config = 3;
            break;
          case AML:
            gain_config = 4;
            break;
          default:
            printf("Epix10kaQuadCache::pixelGainConfig unknown gain control bits %x for pixel (%u, %u)\n", gain_bits, j, k);
            gain_config = 0;
            break;
        }
        pixelGainConfig(i,j,k) = gain_config;
      }
    }
  }

  return pixelGainConfig;
}

DescImage EpixArray::Epix10kaQuadCache::descImage(const Pds::DetInfo& det) const
{
  //
  //   Hard code the array's geometry for now
  //
  //  (Epix10kaQuad)
  //         |
  //  Elem 0 | Elem 1
  //  -------+--------     No rotations
  //  Elem 2 | Elem 3
  //         |
  //
  //  (Elem 0)
  //         |
  //  ASIC 0 | ASIC 3
  //  -------+--------     No rotations
  //  ASIC 1 | ASIC 2
  //         |
  static const SubFrame elem[] = { SubFrame( eM + 4*vM + 1*hM + 2*hE       , eM + 1*vM                      , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 3*hM + 2*hE + 1*wE, eM + 1*vM                      , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 1*hM + 2*hE       , eM + 3*vM        + 1*hE        , wE, hE, D180 ),
                                   SubFrame( eM + 4*vM + 3*hM + 2*hE + 1*wE, eM + 3*vM        + 1*hE        , wE, hE, D180 ) };
  //
  // Determine the bounds of the larger rectangular frame
  //
  unsigned xmin=-1U, xmax=0, ymin=-1U, ymax=0, v;
  for(unsigned i=0; i<numberOfElements(); i++)
    //    if (_config->elemCfg(i).asicMask()) {
    if (1) {
      if ((v=elem[i].x             ) < xmin) xmin = v;
      if ((v=elem[i].x + elem[i].nx) > xmax) xmax = v;
      if ((v=elem[i].y)              < ymin) ymin = v;
      if ((v=elem[i].y + elem[i].ny) > ymax) ymax = v;
    }

  unsigned wImage = xmax - xmin + 2*gM;
  unsigned hImage = ymax - ymin + 2*gM;

  printf("x [%u,%u]  y [%u,%u]  w %u  h %u\n",
         xmin, xmax, ymin, ymax, wImage, hImage);

  //
  // Place each element within the larger rectangular frame
  //
  unsigned ppb=1, dpb=1; // pixels per bin, display ppb
  DescImage desc(det, unsigned(0), ChannelID::name(det),
                 wImage, hImage, ppb, ppb, dpb, dpb);
  for(unsigned i=0; i<numberOfElements(); i++)
    //    if (_config->elemCfg(i).asicMask()) {
    if (1) {
      SubFrame fr(elem[i]);
      fr.x += gM - xmin;
      fr.y += gM - ymin;
#ifdef DBUG
      printf("add_frame x(%d) y(%d) nx(%d) ny(%d)\n",
             fr.x, fr.y, fr.nx, fr.ny);
#endif
      desc.add_frame( fr );
    }
  return desc;
}

ndarray<const uint16_t,3> EpixArray::Epix10kaQuadCache::frame(Pds::TypeId tid, 
                                                              const void* payload,
                                                              FeatureCache& cache,
                                                              const Pds::ClockTime& t) const
{
  const Pds::Epix::ArrayV1* array = reinterpret_cast<const Pds::Epix::ArrayV1*>(payload);
  ndarray<const uint16_t,3> a = array->frame(*_config);
  _envData->fill(cache, array->environmentalRows(*_config));
  _calData->fill(array->calibrationRows(*_config),t);
  return a;
}

static const unsigned offset=1<<16;
//static const unsigned offset=0;
static const double doffset=double(offset);


static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_Epix10ka2MConfig);
  types.push_back(Pds::TypeId::Id_Epix10kaQuadConfig);
  return types;
}


EpixArrayHandler::EpixArrayHandler(const Pds::Src& info,
                                   FeatureCache&   cache) :
  EventHandlerF(info, Pds::TypeId::Id_Epix10kaArray, config_type_list(), cache),
  _desc        ("template",0,0),
  _entry       (0),
  _config_cache(0)
{
}

EpixArrayHandler::~EpixArrayHandler()
{
  if (_config_cache)
    delete _config_cache;
}

unsigned EpixArrayHandler::nentries() const 
{
  unsigned n=0;
  if (_entry) n++;
  if (_config_cache) n += _config_cache->calData()->nentries();
  return n;
}

const Entry* EpixArrayHandler::entry(unsigned i) const 
{
  if (i==0)
    return _entry;
  
  return _config_cache->calData()->entry(i-1);
}

void EpixArrayHandler::rename(const char* s)
{
  printf("rename(%s)  _entry %p\n",s,_entry);
  if (_entry) {
    _entry->desc().name(s);

    _config_cache->envData()->rename(s);
    _config_cache->calData()->rename(s);

#if 0
    int index=0;
    unsigned nAsics       =_config_cache->numberOfAsics();
    for(unsigned a=0; a<nAsics; a++) {
      std::ostringstream ostr;
      ostr << s << ":AsicMonitor" << a;
      _rename_cache(_feature[index++],ostr.str().c_str());
    }

    if (Ami::EventHandler::post_diagnostics())
      for(unsigned a=0; a<16; a++) {
        std::ostringstream ostr;
        ostr << s << ":CommonMode" << _channel_map[a];
        _rename_cache(_feature[index++],ostr.str().c_str());
      }
#endif
  }
}

void EpixArrayHandler::reset() 
{
  _entry = 0; 
  _config_cache->calData()->reset();
}

void EpixArrayHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());

  if (_config_cache) delete _config_cache;
  _config_cache = EpixArray::ConfigCache::instance(tid,payload,*this);
  _config_cache->dump();

  //  Construct Epix elements and place them in the large rectangular frame
  _desc = _config_cache->descImage(det);
  _entry = new EntryImage(_desc);
  _entry->invalid();

  _gain_config = _config_cache->pixelGainConfig();

  _load_pedestals(_desc);
  _load_gains(_desc);
}

void EpixArrayHandler::_calibrate(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t) 
{
  if (!_entry) _configure(tid,payload,t);
}

#include "pdsdata/xtc/ClockTime.hh"

bool EpixArrayHandler::used() const { return (_entry && _entry->desc().used()); }

void EpixArrayHandler::_event    (Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  if (_entry && _entry->desc().used()) {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("EpixArrayHandler::event options %x -> %x\n", _options, o);
      _options = o;
    }

    if (_entry->desc().options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals(_entry->desc());
      _entry->desc().options( _entry->desc().options()&~FrameCalib::option_reload_pedestal() );
    }

    const unsigned fixed_gain_idx = _config_cache->numberOfFixedGainModes() - 1;
    ndarray<const uint16_t,3> frame = _config_cache->frame(tid, payload, _cache, t);
    ndarray<const uint16_t,3> pixelGainConfig = _config_cache->pixelGainConfig();

    _entry->reset();
    const DescImage& d = _entry->desc();

    const unsigned mask = (1<<14)-1;
    const unsigned gain_mask = 3<<14;

    ndarray<unsigned,2> tmp = make_ndarray<unsigned>(hE,wE);
    ndarray<unsigned,2> gsta = make_ndarray<unsigned>(hE,wE);

    //
    //  Apply corrections for each element independently
    //
    for(unsigned i=0; i<frame.shape()[0]; i++) {
#ifdef DBUG
      printf(" frame(%u) x(%u) y(%u) nx(%u) ny(%u)\n",
             i, 
             _desc.frame(i).x, 
             _desc.frame(i).y,
             _desc.frame(i).nx, 
             _desc.frame(i).ny );
#endif
      ndarray<const uint16_t,2> src(frame[i]);
      ndarray<      unsigned,2> dst(_entry->contents(i));
      ndarray<const double,  3> ped(&_pedestals(0,i), _element_ped_shape);
      ndarray<const double,  3> gac(&_gains(0,i), _element_gain_shape);
      ndarray<const unsigned,2> sta(_status[i]);
      ndarray<const uint16_t,2> gcf(_gain_config[i]);

      ped.strides(_element_ped_stride);
      gac.strides(_element_gain_stride);

      //
      //  Apply mask and pedestal
      //
      for(unsigned j=0; j<src.shape()[0]; j++) {
        for(unsigned k=0; k<src.shape()[1]; k++) {
          bool is_switch_mode = false;
          unsigned gain_val = (src(j,k)&gain_mask) >> 14;
          unsigned gain_idx = gcf(j,k);
          double calib_val = (double) ((sta(j,k) ? 0 : (src(j,k)&mask)) + offset);

          // Check if the pixel is in a gain switching mode
          is_switch_mode = gain_idx > fixed_gain_idx;
          // If the pixel is in a gain switching mode update the index
          if (is_switch_mode && gain_val)
            gain_idx += 2;

          if (!(d.options()&FrameCalib::option_no_pedestal()))
            calib_val -= ped(gain_idx,j,k);
          if (calib_val < 0.0) calib_val = 0.0; // mask the problem negative pixels

          gsta(j,k) = sta(j,k) | (is_switch_mode ? gain_val : 0);
          tmp(j,k) = unsigned(calib_val + 0.5);
        }
      }

      //  Apply row common mode
      if (d.options()&FrameCalib::option_correct_common_mode2()) {
        unsigned* ptmp       = tmp.data();
        const unsigned* psta = gsta.data();
        const unsigned shape = wE/8;
        for(unsigned y=0; y<src.size(); y+=wE/8, psta+=wE/8) {
          ndarray<      unsigned,1> s(ptmp, &shape);
          ndarray<const unsigned,1> t(psta, &shape);
          
          unsigned oav = offset*int(d.ppxbin()*d.ppybin()+0.5);
          unsigned olo = oav-100, ohi = oav+100;
          int fn = int(FrameCalib::median(s,t,olo,ohi))-int(oav);
#if 0
          if (Ami::EventHandler::post_diagnostics() && m==0 && y<80)
            _cache.cache(_feature[(y%16)+index],double(fn));
#endif

          for(unsigned z=0; z<wE/8; z++)
            *ptmp++ -= fn;
        }          
      }

      //  Apply channel common mode
      if (d.options()&FrameCalib::option_correct_common_mode()) {
        unsigned shape[2];
        shape[0] = tmp.shape()[0]/2;
        shape[1] = tmp.shape()[1]/8;
        for(unsigned m=0; m<8; m++) {
          ndarray<unsigned,2> s(&tmp((m/4)*shape[0],(m%4)*shape[1]),shape);
          s.strides(tmp.strides());
          ndarray<const unsigned,2> t(&gsta((m/4)*shape[0],(m%4)*shape[1]),shape);
          t.strides(gsta.strides());
          int fn = int(FrameCalib::frameNoise(s,t,offset*int(d.ppxbin()*d.ppybin()+0.5)));
          for(unsigned y=0; y<shape[0]; y++) {
            uint32_t* v = &s(y,0);
            for(unsigned x=0; x<shape[1]; x++)
              v[x] -= fn;
          }
#if 0
          if (Ami::EventHandler::post_diagnostics())
            _cache.cache(_feature[4*k+m+index],double(fn));
#endif
        }
      }

      //  Apply column common mode
      if (d.options()&FrameCalib::option_correct_common_mode3()) {
        for(unsigned y=0; y<tmp.shape()[0]; y+=hE) {
          for(unsigned x=0; x<tmp.shape()[1]; x++) {
            ndarray<uint32_t,1> s(&tmp(y,x),tmp.shape());
            s.strides(tmp.strides());
            ndarray<const uint32_t,1> t(&gsta(y,x),tmp.shape());
            t.strides(tmp.strides());
            unsigned oav = offset*int(d.ppxbin()*d.ppybin()+0.5);
            unsigned olo = oav-100, ohi = oav+100;
            int fn = int(FrameCalib::median(s,t,olo,ohi))-int(oav);
            //if (Ami::EventHandler::post_diagnostics() && m==0 && y<80)
            //  _cache.cache(_feature[(y%16)+index],double(fn));
            for(unsigned k=0; k<hE; k++)
              s[k] -= fn;
          }
        }
      }

      // Apply gain correction hack
      if (d.options()&FrameCalib::option_correct_gain()) {
        for(unsigned j=0; j<src.shape()[0]; j++) {
          for(unsigned k=0; k<src.shape()[1]; k++) {
            unsigned gain_idx = gcf(j,k);
            if ((gain_idx > fixed_gain_idx) && ((src(j,k)&gain_mask) >> 14))
              gain_idx += 2;
            tmp(j,k) = unsigned((tmp(j,k) - doffset) / gac(gain_idx,j,k) + doffset +0.5);
          }
        }
      }

      //
      //  Copy the (rotated) frame into the destination
      //

      switch(_entry->desc().frame(i).r) {
      case D0: 
        for(unsigned j=0; j<src.shape()[0]; j++)
          for(unsigned k=0; k<src.shape()[1]; k++)
            dst(j,k) = tmp(j,k);
        break;
      case D90:
        for(unsigned j=0,jn=src.shape()[0]-1; j<src.shape()[0]; j++,jn--)
          for(unsigned k=0; k<src.shape()[1]; k++)
            dst(k,jn) = tmp(j,k);
        break;
      case D180:
        for(unsigned j=0,jn=src.shape()[0]-1; j<src.shape()[0]; j++,jn--)
          for(unsigned k=0,kn=src.shape()[1]-1; k<src.shape()[1]; k++,kn--)
            dst(jn,kn) = tmp(j,k);
        break;
      case D270:
        for(unsigned j=0; j<src.shape()[0]; j++)
          for(unsigned k=0,kn=src.shape()[1]-1; k<src.shape()[1]; k++,kn--)
            dst(kn,j) = tmp(j,k);
        break;
      default:
        break;
      }
    }

    _entry->info(double(offset*d.ppxbin()*d.ppybin()),EntryImage::Pedestal);
    _entry->info(1.,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void EpixArrayHandler::_damaged() { if (_entry) _entry->invalid(); }

void EpixArrayHandler::_load_pedestals(const DescImage& desc)
{
  //
  //  Load pedestals
  //
  bool use_offline = false;
  unsigned elems  = _config_cache->numberOfElements();
  unsigned gains  = _config_cache->numberOfGainModes();
  unsigned rows   = _config_cache->numberOfRows();
  unsigned cols   = _config_cache->numberOfColumns();

  // Try to find files to fill the status array
  //make_ndarray<double>(gains, elems, rows, cols);
  _status = GainSwitchCalib::load_array(desc.info(), elems, rows, cols, 0, NULL, "sta", "pixel_status");

  // Try loading the pedestal file and see if it is an offline one or not (we need to treat these differently)
  FILE *f = Calib::fopen(desc.info(), "ped", "pedestals", false, &use_offline);

  if (f) {
    if (use_offline) {
      _pedestals = GainSwitchCalib::load_multi_array(desc.info(), gains, elems, rows, cols, 0.0, NULL, f);
    } else {
      unsigned nf = desc.nframes();
      if (nf==0) nf=1;
      unsigned nx = 0;
      unsigned ny = 0;
      for(unsigned i=0; i<nf; i++) {
        const SubFrame& f = desc.frame(i);
        if (f.r== D0 ||
            f.r== D180) {
          nx = f.nx;
          ny = f.ny;
          break;
        }
      }

      _pedestals = make_ndarray<double>(gains,nf,ny,nx);

      //  read pedestals
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      char* pEnd;

      if (nf) {
        for(unsigned s=0; s<nf; s++) {
          const SubFrame& fr = desc.frame(s);
          switch(fr.r) {
          case D0:
            for (unsigned row=0,rown=ny-1; row < ny; row++,rown--) {
              if (feof(f)) return;
              getline(&linep, &sz, f);
              _pedestals(0,s,row,0) = strtod(linep,&pEnd);
              for(unsigned col=1; col<nx; col++)
                _pedestals(0,s,row,col) = strtod(pEnd, &pEnd);
            } break;
          case D90:
            for (unsigned col=0, coln=nx-1; col < nx; col++,coln--) {
              if (feof(f)) return;
              getline(&linep, &sz, f);
              _pedestals(0,s,ny-1,col) = strtod(linep,&pEnd);
              for(unsigned row=1,rown=ny-2; row<ny; row++,rown--)
                _pedestals(0,s,rown,col) = strtod(pEnd, &pEnd);
            } break;
          case D180:
            for (unsigned row=0,rown=ny-1; row < ny; row++,rown--) {
              if (feof(f)) return;
              getline(&linep, &sz, f);
              _pedestals(0,s,rown,nx-1) = strtod(linep,&pEnd);
              for(unsigned col=1; col<nx; col++)
                _pedestals(0,s,rown,nx-col-1) = strtod(pEnd, &pEnd);
            } break;
          case D270:
            for (unsigned col=0, coln=nx-1; col < nx; col++,coln--) {
              if (feof(f)) return;
              getline(&linep, &sz, f);
              _pedestals(0,s,0,coln) = strtod(linep,&pEnd);
              for(unsigned row=1,rown=nx-2; row<ny; row++,rown--)
                _pedestals(0,s,row,coln) = strtod(pEnd, &pEnd);
            } break;
          default: break;
          }
          printf("Ped[0,0]: frame %u: %g\n", s, _pedestals(0,s,0,0));
        }
      }

      free(linep);

      //  Fill in the rest of gain modes with the same data...
      for(unsigned g=1; g<gains; g++)
        for(unsigned i=0; i<elems; i++)
          for(unsigned j=0; j<rows; j++)
            for(unsigned k=0; k<cols; k++)
              _pedestals(g,i,j,k) = _pedestals(0,i,j,k);
    }

    fclose(f);
  } else {
    _pedestals = make_ndarray<double>(gains, elems, rows, cols);
    for(double* val = _pedestals.begin(); val != _pedestals.end(); val++)
      *val = 0.0;
  }

  // Determine the shape and stride for the element specific view of the pedestal data
  _element_ped_shape[0] = _pedestals.shape()[0];
  _element_ped_shape[1] = _pedestals.shape()[2];
  _element_ped_shape[2] = _pedestals.shape()[3];

  _element_ped_stride[0] = _pedestals.strides()[0];
  _element_ped_stride[1] = _pedestals.strides()[2];
  _element_ped_stride[2] = _pedestals.strides()[3];
}

void EpixArrayHandler::_load_gains(const DescImage& desc)
{
  //
  //  Load gain corrections
  //
  bool failed = false;
  unsigned elems  = _config_cache->numberOfElements();
  unsigned gains  = _config_cache->numberOfGainModes();
  unsigned rows   = _config_cache->numberOfRows();
  unsigned cols   = _config_cache->numberOfColumns();
  const unsigned gain_cor[] = { 1, 3, 100, 3, 1, 100, 100 };

  _gains = GainSwitchCalib::load_multi_array(desc.info(), gains, elems, rows, cols, 1.0, &failed, "gain", "pixel_gain");
  if (failed) {
    printf("No valid pixel gain correction file found: using the default corrections!");
    for(unsigned g=0; g<gains; g++)
      for(unsigned i=0; i<elems; i++)
        for(unsigned j=0; j<rows; j++)
          for(unsigned k=0; k<cols; k++)
            _gains(g,i,j,k) = (1.0 / gain_cor[g]);
  }

  // Determine the shape and stride for the element specific view of the pixel gain data
  _element_gain_shape[0] = _gains.shape()[0];
  _element_gain_shape[1] = _gains.shape()[2];
  _element_gain_shape[2] = _gains.shape()[3];

  _element_gain_stride[0] = _gains.strides()[0];
  _element_gain_stride[1] = _gains.strides()[2];
  _element_gain_stride[2] = _gains.strides()[3];
}

EpixArray::EnvData2M::EnvData2M(Ami::EventHandlerF& h) : 
  _name(4), _quad(4) 
{
  for(unsigned i=0; i<_quad.size(); i++)
    _quad[i] = new EpixArray::EnvDataQuad(h);
}

EpixArray::EnvData2M::~EnvData2M()
{
  for(unsigned i=0; i<_quad.size(); i++)
    delete _quad[i];
}

void EpixArray::EnvData2M::addFeatures(const char* name)
{
  for(unsigned e=0; e<_quad.size(); e++) {
    std::ostringstream ostr;
    ostr << name << ":Quad[" << e << "]";
    _quad[e]->addFeatures(ostr.str().c_str());
  }
}

void EpixArray::EnvData2M::rename(const char* name)
{
  for(unsigned q=0; q<_quad.size(); q++) {
    std::ostringstream ostr;
    ostr << name << ":Quad[" << q << "]";
    _quad[q]->rename(ostr.str().c_str());
  }
}


void EpixArray::EnvData2M::fill(FeatureCache& cache,
                                ndarray<const uint32_t,3> env)
{
  for(unsigned e=0; e<_quad.size(); e++) {
    ndarray<const uint32_t,2> a(env[4*e+2]);
    _quad[e]->_fill(cache,a);
  }
}


EpixArray::EnvDataQuad::EnvDataQuad(EventHandlerF& handler) :
  _handler(handler), _index(make_ndarray<int>(38))
{
}

void EpixArray::EnvDataQuad::addFeatures(const char* name)
{
#define ADDV(s) {                                                       \
    std::ostringstream ostr;                                            \
    ostr << name << ":" << #s;                                          \
      _index[index++] = _handler._add_to_cache(ostr.str().c_str()); }
  
  unsigned index=0;
  ADDV(Sht31_Hum);                                
  ADDV(Sht31_TempC);                                
  ADDV(NctLoc_TempC);                             
  ADDV(NctFpga_TempC);                              
  ADDV(A0_2V5_Curr_mA);                             
  ADDV(A1_2V5_Curr_mA);                               
  ADDV(A2_2V5_Curr_mA);                               
  ADDV(A3_2V5_Curr_mA);                               
  ADDV(D0_2V5_Curr_mA);                               
  ADDV(D1_2V5_Curr_mA);                               
  ADDV(Therm0_TempC);                                
  ADDV(Therm1_TempC);                              
  ADDV(PwrDig_CurrA);                             
  ADDV(PwrDig_Vin);                             
  ADDV(PwrDig_TempC);                             
  ADDV(PwrAna_CurrA);                             
  ADDV(PwrAna_Vin);                             
  ADDV(PwrAna_TempC);                             
  ADDV(A0_2V5_H_TempC);                             
  ADDV(A0_2V5_L_TempC);                             
  ADDV(A1_2V5_H_TempC);                             
  ADDV(A1_2V5_L_TempC);                             
  ADDV(A2_2V5_H_TempC);                             
  ADDV(A2_2V5_L_TempC);                             
  ADDV(A3_2V5_H_TempC);                             
  ADDV(A3_2V5_L_TempC);                             
  ADDV(D0_2V5_TempC);                             
  ADDV(D1_2V5_TempC);                             
  ADDV(A0_1V8_TempC);                             
  ADDV(A1_1V8_TempC);                             
  ADDV(A2_1V8_TempC);                             
  ADDV(PcbAna_Temp0C);
  ADDV(PcbAna_Temp1C);
  ADDV(PcbAna_Temp2C);
  ADDV(TrOpt_TempC);
  ADDV(TrOpt_Vcc);
  ADDV(TrOpt_TxPwr_uW);
  ADDV(TrOpt_RxPwr_uW);

#undef ADDV
}

void EpixArray::EnvDataQuad::rename(const char* name)
{
#define ADDV(s) {                                                       \
    std::ostringstream ostr;                                            \
    ostr << name << ":" << #s;                                          \
      _handler._rename_cache(_index[index++],ostr.str().c_str()); }

  unsigned index=0;
  ADDV(Sht31_Hum);                                
  ADDV(Sht31_TempC);                                
  ADDV(NctLoc_TempC);                             
  ADDV(NctFpga_TempC);                              
  ADDV(A0_2V5_Curr_mA);                             
  ADDV(A1_2V5_Curr_mA);                               
  ADDV(A2_2V5_Curr_mA);                               
  ADDV(A3_2V5_Curr_mA);                               
  ADDV(D0_2V5_Curr_mA);                               
  ADDV(D1_2V5_Curr_mA);                               
  ADDV(Therm0_TempC);                                
  ADDV(Therm1_TempC);                              
  ADDV(PwrDig_CurrA);                             
  ADDV(PwrDig_Vin);                             
  ADDV(PwrDig_TempC);                             
  ADDV(PwrAna_CurrA);                             
  ADDV(PwrAna_Vin);                             
  ADDV(PwrAna_TempC);                             
  ADDV(A0_2V5_H_TempC);                             
  ADDV(A0_2V5_L_TempC);                             
  ADDV(A1_2V5_H_TempC);                             
  ADDV(A1_2V5_L_TempC);                             
  ADDV(A2_2V5_H_TempC);                             
  ADDV(A2_2V5_L_TempC);                             
  ADDV(A3_2V5_H_TempC);                             
  ADDV(A3_2V5_L_TempC);                             
  ADDV(D0_2V5_TempC);                             
  ADDV(D1_2V5_TempC);                             
  ADDV(A0_1V8_TempC);                             
  ADDV(A1_1V8_TempC);                             
  ADDV(A2_1V8_TempC);                             
  ADDV(PcbAna_Temp0C);
  ADDV(PcbAna_Temp1C);
  ADDV(PcbAna_Temp2C);
  ADDV(TrOpt_TempC);
  ADDV(TrOpt_Vcc);
  ADDV(TrOpt_TxPwr_uW);
  ADDV(TrOpt_RxPwr_uW);

#undef ADDV
}

static double getThermistorTemp(const uint16_t x)
{
  if (x==0) return 0.;
  double u = double(x)/16383.0 * 2.5;
  double i = u / 100000;
  double r = (2.5 - u)/i;
  double l = log(r/10000);
  double t = 1.0 / (3.3538646E-03 + 2.5654090E-04 * l + 1.9243889E-06 * (l*l) + 1.0969244E-07 * (l*l*l));
  return t - 273.15;
}

void EpixArray::EnvDataQuad::fill(FeatureCache& cache,
                                  ndarray<const uint32_t,3> env)
{
  _fill(cache,ndarray<const uint32_t,2>(env[2]));
}

void EpixArray::EnvDataQuad::_fill(FeatureCache& cache,
                                   ndarray<const uint32_t,2> env)
{
  uint16_t data[38];
  ndarray<const uint32_t,1> a(env[0]);
  ndarray<const uint32_t,1>::reverse_iterator iter = a.rbegin();
  for(unsigned i=0; i<38; ) {
    data[i++] = (*iter)&0xffff;
    data[i++] = (*iter)>>16;
    iter++;
  }
  unsigned index=0;
  cache.cache(_index[index++], double(data[0])/65535.0 * 100);
  cache.cache(_index[index++], double(data[1])/65535.0 * 175 - 45);
  cache.cache(_index[index++], double(data[2]&0xff));
  cache.cache(_index[index++], double(data[3]>>8)+double(data[3]&0xc0)/256.);
  cache.cache(_index[index++], double(data[4])/16383.0*2.5/330.*1.e6);
  cache.cache(_index[index++], double(data[5])/16383.0*2.5/330.*1.e6);
  cache.cache(_index[index++], double(data[6])/16383.0*2.5/330.*1.e6);
  cache.cache(_index[index++], double(data[7])/16383.0*2.5/330.*1.e6);
  cache.cache(_index[index++], double(data[8])/16383.0*2.5/330.*0.5e6);
  cache.cache(_index[index++], double(data[9])/16383.0*2.5/330.*0.5e6);
  cache.cache(_index[index++], getThermistorTemp(data[10]));
  cache.cache(_index[index++], getThermistorTemp(data[11]));
  cache.cache(_index[index++], double(data[12])*0.1024/4095/0.02);
  cache.cache(_index[index++], double(data[13])*102.4/4095);
  cache.cache(_index[index++], double(data[14])*2.048/4095*(130/(0.882-1.951)) + (0.882/0.0082+100));
  cache.cache(_index[index++], double(data[15])*0.1024/4095/0.02);
  cache.cache(_index[index++], double(data[16])*102.4/4095);
  cache.cache(_index[index++], double(data[17])*2.048/4095*(130/(0.882-1.951)) + (0.882/0.0082+100));
  for(unsigned i=0; i<13; i++)
    cache.cache(_index[index++], double(data[18+i])*1.65/65535*100);
  cache.cache(_index[index++], double(data[31])*1.65/65535*(130/0.882-1.951)+(0.882/0.0082+100));
  cache.cache(_index[index++], double(data[32])*1.65/65535*(130/0.882-1.951)+(0.882/0.0082+100));
  cache.cache(_index[index++], double(data[33])*1.65/65535*(130/0.882-1.951)+(0.882/0.0082+100));
  cache.cache(_index[index++], double(data[34])/256);
  cache.cache(_index[index++], double(data[35])*0.0001);
  cache.cache(_index[index++], double(data[36])*0.1);
  cache.cache(_index[index++], double(data[37])*0.1);
}

EpixArray::CalData::~CalData()
{
  reset();
}

void EpixArray::CalData::config(const Pds::DetInfo& det,
                                unsigned numberOfRows,
                                unsigned numberOfColumns)
{
  if (!numberOfRows) return;

  _elem.resize(_name.size()*numberOfRows);
  unsigned ch=0;
  for(unsigned i=0; i<_name.size(); i++) {
    for(unsigned j=0; j<numberOfRows; j++) {
      DescWaveform desc(det, ch,
                        ChannelID::name(det,ch),
                        "Column", "Value",
                        numberOfColumns, 0., float(numberOfColumns));
      _elem[ch++] = new EntryWaveform(desc);
    }
  }
  Channel chm((1<<ch)-1, Channel::BitMask);
  std::string s(ChannelID::name(det,chm));
  s += "-Cal";
  _ref = new EntryRef(DescRef(det,chm,s.c_str()));
  _ref->set(_elem.data());
}

void     EpixArray::CalData::rename     (const char* name)
{
  if (!_ref) return;
     
  _rename(name);

  std::ostringstream ostr;
  ostr << name << "-Cal";
  _ref->desc().name(ostr.str().c_str());
}

void EpixArray::CalData::reset()
{
  _ref = 0;

  for(unsigned i=0; i<_elem.size(); i++)
    delete _elem[i];
  _elem.clear();
}

void     EpixArray::CalData::fill       (ndarray<const uint16_t,3> cal,
                                         const Pds::ClockTime& t)
{
  unsigned ch=0;
  for(unsigned e=0; e<cal.shape()[0]; e++) {
    for(unsigned i=0; i<cal.shape()[1]; i++) {
      EntryWaveform* entry = _elem[ch++];
      const uint16_t* v = &cal(e,i,0);
      for(unsigned j=0; j<cal.shape()[2]; j++)
        entry->content(double(*v++),j);
      entry->info(1,EntryWaveform::Normalization);
      entry->valid(t);
    }
  }
  if (_ref)
    _ref->valid(t);
}

EpixArray::CalDataQuad::CalDataQuad() : CalData(4) {}
 
EpixArray::CalData2M::CalData2M() : CalData(16) {}
