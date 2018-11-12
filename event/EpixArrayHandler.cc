#include "EpixArrayHandler.hh"

#include "ami/event/FrameCalib.hh"
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

typedef Pds::Epix::Config10ka2MV1   Cfg10ka2M;
typedef Pds::Epix::Config10kaQuadV1 Cfg10kaQuad;
typedef Pds::Epix::Config10ka       Cfg10ka;

static const unsigned      wElem = Cfg10ka::_numberOfPixelsPerAsicRow*Cfg10ka::_numberOfAsicsPerRow;
static const unsigned      hElem = Cfg10ka::_numberOfRowsPerAsic     *Cfg10ka::_numberOfAsicsPerColumn;

using Ami::EventHandlerF;
using Ami::FeatureCache;
using Ami::Entry;
using Ami::EntryRef;
using Ami::EntryWaveform;

namespace EpixArray {

  class EnvDataElem {
  public:
    EnvDataElem(Ami::EventHandlerF&);
  public:
    void     addFeatures(const char* name);
    void     rename     (const char* name);
    void     fill       (FeatureCache&, ndarray<const uint32_t,2> env);
  private:
    EventHandlerF& _handler;
    ndarray<int,1> _index;
  };

  class EnvData {
  public:
    EnvData(unsigned n, Ami::EventHandlerF&);
    virtual ~EnvData();
    void     addFeatures(const char*   name);
    void     rename     (const char*   name);
    void     fill       (FeatureCache&, ndarray<const uint32_t,3> env);
  private:
    virtual void _rename(const char* name)=0;
  protected:
    std::vector<std::string > _name;
    std::vector<EnvDataElem*> _elem;
  };

  class EnvDataQuad : public EnvData {
  public:
    EnvDataQuad(Ami::EventHandlerF&);
  private:
    void _rename(const char*);
  };

  class EnvData2M : public EnvData {
  public:
    EnvData2M(EventHandlerF&);
  private:
    void _rename(const char*);
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
    virtual unsigned numberOfRowsCal  () const = 0;
    
    virtual Ami::DescImage descImage  (const Pds::DetInfo&) const = 0;
    virtual EnvData*       envData    () const = 0;
    virtual CalData*       calData    () const = 0;
    virtual ndarray<const uint16_t,3> frame(Pds::TypeId, const void*, FeatureCache&, const Pds::ClockTime&) const = 0;

    virtual void dump() const = 0;
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
    unsigned numberOfRowsCal  () const { return _config->numberOfCalibrationRows(); }
    
    Ami::DescImage descImage  (const Pds::DetInfo&) const;
    EnvData*       envData    () const { return _envData; }
    CalData*       calData    () const { return _calData; }
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
    unsigned numberOfRowsCal  () const { return _config->numberOfCalibrationRows(); }
    
    Ami::DescImage descImage  (const Pds::DetInfo&) const;
    EnvData*       envData    () const { return _envData; }
    CalData*       calData    () const { return _calData; }
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

DescImage EpixArray::Epix10ka2MCache::descImage(const Pds::DetInfo& det) const
{
  //
  //   Hard code the array's geometry for now
  //
  //  (Epix10ka2m)
  //         |
  //  Quad 3 | Quad 0      Quad 1 is rotated  90d clockwise
  //  -------+--------     Quad 2 is rotated 180d clockwise
  //  Quad 2 | Quad 1      Quad 3 is rotated 270d clockwise
  //         |
  //
  //  (Quad 0)
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
  static const unsigned      eMargin = 4;
  static const SubFrame elem[] = { SubFrame( 3*eMargin + 2*hElem        , 1*eMargin                  , wElem, hElem, D0 ),
                                   SubFrame( 4*eMargin + 2*hElem + wElem, 1*eMargin                  , wElem, hElem, D0 ),
                                   SubFrame( 3*eMargin + 2*hElem        , 2*eMargin + 1*hElem        , wElem, hElem, D0 ),
                                   SubFrame( 4*eMargin + 2*hElem + wElem, 2*eMargin + 1*hElem        , wElem, hElem, D0 ),
                                   SubFrame( 4*eMargin + 2*wElem + hElem, 3*eMargin + 2*hElem        , hElem, wElem, D90 ),
                                   SubFrame( 4*eMargin + 2*wElem + hElem, 4*eMargin + 2*hElem + wElem, hElem, wElem, D90 ),
                                   SubFrame( 3*eMargin + 2*wElem        , 3*eMargin + 2*hElem        , hElem, wElem, D90 ),
                                   SubFrame( 3*eMargin + 2*wElem        , 4*eMargin + 2*hElem + wElem, hElem, wElem, D90 ),
                                   SubFrame( 2*eMargin + 1*wElem        , 4*eMargin + 2*wElem + hElem, wElem, hElem, D180 ),
                                   SubFrame( 1*eMargin                  , 4*eMargin + 2*wElem + hElem, wElem, hElem, D180 ),
                                   SubFrame( 2*eMargin + 1*wElem        , 3*eMargin + 2*wElem        , wElem, hElem, D180 ),
                                   SubFrame( 1*eMargin                  , 3*eMargin + 2*wElem        , wElem, hElem, D180 ),
                                   SubFrame( 1*eMargin                  , 2*eMargin + 1*wElem        , hElem, wElem, D270 ),
                                   SubFrame( 1*eMargin                  , 1*eMargin                  , hElem, wElem, D270 ),
                                   SubFrame( 2*eMargin + 1*hElem        , 2*eMargin + 1*wElem        , hElem, wElem, D270 ),
                                   SubFrame( 2*eMargin + 1*hElem        , 1*eMargin                  , hElem, wElem, D270 ) };
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

  unsigned wImage = xmax - xmin + 2*eMargin;
  unsigned hImage = ymax - ymin + 2*eMargin;

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
      fr.x += eMargin - xmin;
      fr.y += eMargin - ymin;
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
  static const unsigned      eMargin = 4;
  static const SubFrame elem[] = { SubFrame( 3*eMargin + 2*hElem        , 1*eMargin                  , wElem, hElem, D0 ),
                                   SubFrame( 4*eMargin + 2*hElem + wElem, 1*eMargin                  , wElem, hElem, D0 ),
                                   SubFrame( 3*eMargin + 2*hElem        , 2*eMargin + 1*hElem        , wElem, hElem, D0 ),
                                   SubFrame( 4*eMargin + 2*hElem + wElem, 2*eMargin + 1*hElem        , wElem, hElem, D0 ) };
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

  unsigned wImage = xmax - xmin + 2*eMargin;
  unsigned hImage = ymax - ymin + 2*eMargin;

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
      fr.x += eMargin - xmin;
      fr.y += eMargin - ymin;
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

  _load_pedestals(_desc);
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

    ndarray<const uint16_t,3> frame = _config_cache->frame(tid, payload, _cache, t);
    _entry->reset();
    const DescImage& d = _entry->desc();

    const ndarray<const unsigned,3>& pa =
      d.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pedestals;

    const unsigned mask = (1<<14)-1;

    ndarray<unsigned,2> tmp = make_ndarray<unsigned>(hElem,wElem);

    //
    //  Apply corrections for each element independently
    //
    for(unsigned i=0; i<frame.shape()[0]; i++) {
      ndarray<const uint16_t,2> src(frame[i]);
      ndarray<      unsigned,2> dst(_entry->contents(i));
      ndarray<const unsigned,2> ped(pa[i]);
      ndarray<const unsigned,2> sta(_status[i]);

      //
      //  Apply mask and pedestal
      //
      for(unsigned j=0; j<src.shape()[0]; j++)
        for(unsigned k=0; k<src.shape()[1]; k++)
          tmp(j,k) = (src(j,k)&mask) + ped(j,k);

      //  Apply row common mode
      if (d.options()&FrameCalib::option_correct_common_mode2()) {
        unsigned* ptmp       = tmp.data();
        const unsigned* psta = sta.data();
        const unsigned shape = wElem/8;
        for(unsigned y=0; y<src.size(); y+=wElem/8, psta+=wElem/8) {
          ndarray<      unsigned,1> s(ptmp, &shape);
          ndarray<const unsigned,1> t(psta, &shape);
          
          unsigned oav = offset*int(d.ppxbin()*d.ppybin()+0.5);
          unsigned olo = oav-100, ohi = oav+100;
          int fn = int(FrameCalib::median(s,t,olo,ohi))-int(oav);
#if 0
          if (Ami::EventHandler::post_diagnostics() && m==0 && y<80)
            _cache.cache(_feature[(y%16)+index],double(fn));
#endif

          for(unsigned z=0; z<wElem/8; z++)
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
          ndarray<const unsigned,2> t(&sta((m/4)*shape[0],(m%4)*shape[1]),shape);
          t.strides(sta.strides());
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
        for(unsigned y=0; y<tmp.shape()[0]; y+=hElem) {
          for(unsigned x=0; x<tmp.shape()[1]; x++) {
            ndarray<uint32_t,1> s(&tmp(y,x),tmp.shape());
            s.strides(tmp.strides());
            ndarray<const uint32_t,1> t(&sta(y,x),tmp.shape());
            t.strides(tmp.strides());
            unsigned oav = offset*int(d.ppxbin()*d.ppybin()+0.5);
            unsigned olo = oav-100, ohi = oav+100;
            int fn = int(FrameCalib::median(s,t,olo,ohi))-int(oav);
            //if (Ami::EventHandler::post_diagnostics() && m==0 && y<80)
            //  _cache.cache(_feature[(y%16)+index],double(fn));
            for(unsigned k=0; k<hElem; k++)
              s[k] -= fn;
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
  unsigned nf = desc.nframes();
  if (nf==0) nf=1;
  unsigned nx = 0;
  unsigned ny = 0;
  for(unsigned i=0; i<nf; i++) {
    const SubFrame& f = desc.frame(i);
    if (f.r== D0) {
      nx = f.nx;
      ny = f.ny;
      break;
    }
  }
  _pedestals = make_ndarray<unsigned>(nf,ny,nx);
  _offset    = make_ndarray<unsigned>(nf,ny,nx);
  _status    = make_ndarray<unsigned>(nf,ny,nx);
  for(unsigned i=0; i<nf; i++)
    for(unsigned j=0; j<ny; j++)
      for(unsigned k=0; k<nx; k++) {
	_offset(i,j,k) = offset;
        _status(i,j,k) = 0;
      }

  //
  //  Load pedestals
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];
    
  sprintf(oname1,"ped.%08x.dat",desc.info().phy());
  sprintf(oname2,"/reg/g/pcds/pds/framecalib/ped.%08x.dat",desc.info().phy());
  FILE *f = Calib::fopen_dual(oname1, oname2, "pedestals");

  if (f) {

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
            _pedestals(s,row,0) = offset-strtoul(linep,&pEnd,0);
            for(unsigned col=1; col<nx; col++) 
              _pedestals(s,row,col) = offset-strtoul(pEnd, &pEnd,0);
          } break;
        case D90:
          for (unsigned col=0, coln=nx-1; col < nx; col++,coln--) {
            if (feof(f)) return;
            getline(&linep, &sz, f);
            _pedestals(s,ny-1,col) = offset-strtoul(linep,&pEnd,0);
            for(unsigned row=1,rown=ny-2; row<ny; row++,rown--) 
              _pedestals(s,rown,col) = offset-strtoul(pEnd, &pEnd,0);
          } break;
        case D180:
          for (unsigned row=0,rown=ny-1; row < ny; row++,rown--) {
            if (feof(f)) return;
            getline(&linep, &sz, f);
            _pedestals(s,rown,nx-1) = offset-strtoul(linep,&pEnd,0);
            for(unsigned col=1; col<nx; col++) 
              _pedestals(s,rown,nx-col-1) = offset-strtoul(pEnd, &pEnd,0);
          } break;
        case D270:
          for (unsigned col=0, coln=nx-1; col < nx; col++,coln--) {
            if (feof(f)) return;
            getline(&linep, &sz, f);
            _pedestals(s,0,coln) = offset-strtoul(linep,&pEnd,0);
            for(unsigned row=1,rown=nx-2; row<ny; row++,rown--) 
              _pedestals(s,row,coln) = offset-strtoul(pEnd, &pEnd,0);
          } break;
        default: break;
        }
        printf("Ped[0,0]: frame %u: %08x\n", s, _pedestals(s,0,0));
      }    
    }
      
    free(linep);
    fclose(f);
  }
  else {
    for(unsigned i=0; i<nf; i++)
      for(unsigned j=0; j<ny; j++)
	for(unsigned k=0; k<nx; k++)
	  _pedestals(i,j,k) = offset;
  }
}


EpixArray::EnvData::EnvData(unsigned n, Ami::EventHandlerF& h) : 
  _name(n), _elem(n) 
{
  for(unsigned i=0; i<n; i++)
    _elem[i] = new EpixArray::EnvDataElem(h);
}

EpixArray::EnvData::~EnvData()
{
  for(unsigned i=0; i<_elem.size(); i++)
    delete _elem[i];
}

void EpixArray::EnvData::addFeatures(const char* name)
{
  _rename(name);
  for(unsigned e=0; e<_elem.size(); e++)
    _elem[e]->addFeatures(_name[e].c_str());
}

void EpixArray::EnvData::rename(const char* name)
{
  _rename(name);
  for(unsigned e=0; e<_elem.size(); e++)
    _elem[e]->rename(_name[e].c_str());
}


void EpixArray::EnvData::fill(FeatureCache& cache,
                              ndarray<const uint32_t,3> env)
{
  for(unsigned e=0; e<env.shape()[0]; e++) {
    ndarray<const uint32_t,2> a(env[e]);
    _elem[e]->fill(cache,a);
  }
}


EpixArray::EnvData2M::EnvData2M(EventHandlerF& handler) : EnvData(16, handler)
{
}

void EpixArray::EnvData2M::_rename(const char* name)
{
  unsigned i=0;
  for(unsigned q=0; q<4; q++) 
    for(unsigned e=0; e<4; e++) {
      std::ostringstream ostr;
      ostr << name << ":Quad[" << q << "]:Elem[" << e << "]";
      _name[i++] = ostr.str();
    }
}



EpixArray::EnvDataQuad::EnvDataQuad(EventHandlerF& handler) : EnvData(4, handler)
{
}

void EpixArray::EnvDataQuad::_rename(const char* name)
{
  unsigned i=0;
  for(unsigned q=0; q<4; q++) {
    std::ostringstream ostr;
    ostr << name << ":Elem[" << q << "]";
    _name[i++] = ostr.str();
  }
}


EpixArray::EnvDataElem::EnvDataElem(EventHandlerF& handler) :
  _handler(handler), _index(make_ndarray<int>(9))
{
}

void EpixArray::EnvDataElem::addFeatures(const char* name)
{
#define ADDV(s) {                                                       \
    std::ostringstream ostr;                                            \
    ostr << name << ":" << #s;                                          \
      _index[index++] = _handler._add_to_cache(ostr.str().c_str()); }
  
  unsigned index=0;
  ADDV(Temp0);                                
  ADDV(Temp1);                                
  ADDV(Humidity);                             
  ADDV(AnalogI);                              
  ADDV(DigitalI);                             
  ADDV(GuardI);                               
  ADDV(BiasI);                                
  ADDV(AnalogV);                              
  ADDV(DigitalV);                             

#undef ADDV
}

void EpixArray::EnvDataElem::rename(const char* name)
{
#define ADDV(s) {                                                       \
    std::ostringstream ostr;                                            \
    ostr << name << ":" << #s;                                          \
      _handler._rename_cache(_index[index++],ostr.str().c_str()); }

  unsigned index=0;
  ADDV(Temp0);                                
  ADDV(Temp1);                                
  ADDV(Humidity);                             
  ADDV(AnalogI);                              
  ADDV(DigitalI);                             
  ADDV(GuardI);                               
  ADDV(BiasI);                                
  ADDV(AnalogV);                              
  ADDV(DigitalV);                             

#undef ADDV
}

void EpixArray::EnvDataElem::fill(FeatureCache& cache,
                                  ndarray<const uint32_t,2> env)
{
  const uint32_t* data = &env(env.shape()[0]-1,0);
  const int32_t* sdata = reinterpret_cast<const int32_t*>(data);
  unsigned index=0;
  cache.cache(_index[index++], double(sdata[0])*0.01);
  cache.cache(_index[index++], double(sdata[1])*0.01);
  cache.cache(_index[index++], double(sdata[2])*0.01);
  cache.cache(_index[index++], double(data[3])*0.001);
  cache.cache(_index[index++], double(data[4])*0.001);
  cache.cache(_index[index++], double(data[5])*0.000001);
  cache.cache(_index[index++], double(data[6])*0.000001);
  cache.cache(_index[index++], double(data[7])*0.001);
  cache.cache(_index[index++], double(data[8])*0.001);
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
