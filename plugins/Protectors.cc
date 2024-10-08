#include "Protectors.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/DescScan.hh"

#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/psddl/jungfrau.ddl.h"
#include "pdsdata/psddl/epix.ddl.h"

#include "ami/plugins/BlackHole.hh"
#include "ami/plugins/ProtectionIOC.hh"

#include <cstdio>

static const char* CspadName = "CspadProtect";
static const char* JungfrauName = "JungfrauProtect";
static const char* EpixArrayName = "EpixArrayProtect";
static const char* Epix10kaName = "Epix10kaProtect";
static const char* EpixName = "EpixProtect";

typedef Pds::Jungfrau::ConfigV3 JfCfgV3;
typedef Pds::Jungfrau::ConfigV4 JfCfgV4;
typedef Pds::Jungfrau::ElementV2 JfDataV2;
typedef Pds::Epix::Config100aV1 EpixCfg100aV1;
typedef Pds::Epix::Config100aV2 EpixCfg100aV2;
typedef Pds::Epix::Config10kaV1 EpixCfg10kaV1;
typedef Pds::Epix::Config10kaV2 EpixCfg10kaV2;
typedef Pds::Epix::Elem10kaConfigV1 EpixCfg10ka;
typedef Pds::Epix::Config10kaQuadV1 EpixCfg10kaQuadV1;
typedef Pds::Epix::Config10kaQuadV2 EpixCfg10kaQuadV2;
typedef Pds::Epix::Config10ka2MV1 EpixCfg10ka2MV1;
typedef Pds::Epix::Config10ka2MV2 EpixCfg10ka2MV2;
typedef Pds::Epix::ElementV2 EpixDataV2;
typedef Pds::Epix::ElementV3 EpixDataV3;
typedef Pds::Epix::ArrayV1 EpixArrayV1;

using namespace Ami;

Protector::Protector(const char* pname,
                     const Pds::DetInfo& info,
                     const PVHandler* handler,
                     bool uses_bh) :
  _pname(pname),
  _name(Pds::DetInfo::name(info)),
  _info(info),
  _npixel_entry(NULL),
  _npixel_scan(NULL),
  _tripped_scan(NULL),
  _handler(handler),
  _uses_bh(uses_bh),
  _nevt(0),
  _lastTrip(Pds::ClockTime(0,0)),
  _cache(NULL),
  _npoints_index(0),
  _tripped_index(0),
  _evttime_index(0)
{}

Protector::~Protector()
{
  if (_npixel_entry) {
    delete _npixel_entry;
  }
  if (_npixel_scan) {
    delete _npixel_scan;
  }
  if (_tripped_scan) {
    delete _tripped_scan;
  }
}

Protector* Protector::instance(const Pds::DetInfo& info,
                               const Pds::TypeId& type,
                               void* payload,
                               const Threshold* threshold)
{
  Protector* prot = NULL;
  const PVHandler* handler = threshold->lookup(info, type);
  // if there is a threshold handler for detector run the protection
  if (handler) {
    if (type.id()==Pds::TypeId::Id_CspadConfig) {
      switch(type.version()) {
        case 5:
          prot = new CsPadProtector(info, type, payload, handler);
          break;
        default:
          printf("%s unsupported Cspad version: %u\n",
                 CspadName,
                 type.version());
      }
    } else if (type.id()==Pds::TypeId::Id_JungfrauConfig) {
      switch(type.version()) {
        case 3:
          prot = new JungfrauProtector<JfCfgV3,JfDataV2>(info, type, payload, handler);
          break;
        case 4:
          prot = new JungfrauProtector<JfCfgV4,JfDataV2>(info, type, payload, handler);
          break;
        default:
          printf("%s unsupported Jungfrau version: %u\n",
                 JungfrauName,
                 type.version());
      }
    } else if (type.id()==Pds::TypeId::Id_Epix10kaQuadConfig) {
      switch(type.version()) {
        case 2:
          prot = new EpixArrayProtector<EpixCfg10kaQuadV2, EpixArrayV1>(info, type, payload, handler);
          break;
        case 1:
          prot = new EpixArrayProtector<EpixCfg10kaQuadV1, EpixArrayV1>(info, type, payload, handler);
          break;
        default:
          printf("%s unsupported Epix version: %u\n",
                 EpixArrayName,
                 type.version());
      }
    } else if (type.id()==Pds::TypeId::Id_Epix10ka2MConfig) {
      switch(type.version()) {
        case 2:
          prot = new EpixArrayProtector<EpixCfg10ka2MV2, EpixArrayV1>(info, type, payload, handler);
          break;
        case 1:
          prot = new EpixArrayProtector<EpixCfg10ka2MV1, EpixArrayV1>(info, type, payload, handler);
          break;
        default:
          printf("%s unsupported Epix version: %u\n",
                 EpixArrayName,
                 type.version());
      }
    } else if (type.id()==Pds::TypeId::Id_Epix10kaConfig) {
      switch(type.version()) {
        case 2:
          prot = new Epix10kaProtector<EpixCfg10kaV2, EpixDataV3>(info, type, payload, handler);
          break;
        case 1:
          prot = new Epix10kaProtector<EpixCfg10kaV1, EpixDataV3>(info, type, payload, handler);
          break;
        default:
          printf("%s unsupported Epix version: %u\n",
                 Epix10kaName,
                 type.version());
      }
    } else if (type.id()==Pds::TypeId::Id_Epix100aConfig) {
      switch(type.version()) {
        case 2:
          prot = new EpixProtector<EpixCfg100aV2, EpixDataV3>(info, type, payload, handler);
          break;
        case 1:
          prot = new EpixProtector<EpixCfg100aV1, EpixDataV2>(info, type, payload, handler);
          break;
        default:
          printf("%s unsupported Epix version: %u\n",
                 EpixName,
                 type.version());
      }
    }
  }
  return prot;
}

const Pds::DetInfo& Protector::info() const
{
  return _info;
}

void Protector::clear()
{
  if (_npixel_entry) {
    delete _npixel_entry;
    _npixel_entry = NULL;
  }
  if (_npixel_scan) {
    delete _npixel_scan;
    _npixel_scan = NULL;
  }
  if (_tripped_scan) {
    delete _tripped_scan;
    _tripped_scan = NULL;
  }
}

void Protector::setName(const char* name)
{
  if (name) {
    _name.assign(name);
  }
}

bool Protector::hasNumPixelEntry() const
{
  return _npixel_entry != NULL;
}

bool Protector::hasNumPixelScan() const
{
  return _npixel_scan != NULL;
}

bool Protector::hasTrippedScan() const
{
  return _tripped_scan != NULL;
}

EntryScalar* Protector::numPixelEntry()
{
  std::string title = "Pixels over threshold#" + _name + "#0#0";
  if (!_npixel_entry) {
    _npixel_entry = new EntryScalar(DescScalar(title.c_str(), "npixels"));
  }

  return _npixel_entry;
}

EntryScan* Protector::numPixelScan()
{
  std::string title = "Pixels over threshold vs event#" + _name + "#0#1";
  if (!_npixel_scan) {
    _npixel_scan = new EntryScan(DescScan(title.c_str(), "event time", "npixels", 10000));
  }

  return _npixel_scan;
}

EntryScan* Protector::trippedScan()
{
  std::string title = "Tripped state vs event#" + _name + "#0#3";
  if (!_tripped_scan) {
    _tripped_scan = new EntryScan(DescScan(title.c_str(), "event time", "tripped", 10000));
  }

  return _tripped_scan;
}

void Protector::cache(FeatureCache* cache)
{
  _cache = cache;
  _npoints_index = _cache->add(_name+":protect:npixels");
  _tripped_index = _cache->add(_name+":protect:tripped");
}

void Protector::accept(const Pds::ClockTime& clk)
{
  if (!ready()) return;

  bool trip = false;
  int32_t pixelCount = 0;

  // call the detector specific analysis code
  trip = analyzeDetector(clk, pixelCount);

  if (_npixel_entry) {
    _npixel_entry->addcontent(pixelCount);
    _npixel_entry->valid(clk);
  }

  if (_npixel_scan) {
    _npixel_scan->addy(pixelCount, clk.asDouble());
    _npixel_scan->valid(clk);
  }

  if (_tripped_scan) {
    _tripped_scan->addy(trip, clk.asDouble());
    _tripped_scan->valid(clk);
  }

  if (_cache) {
    _cache->cache(_npoints_index, pixelCount);
    _cache->cache(_tripped_index, trip);
  }

  if (trip) {
    float deltaT = clk.asDouble() - _lastTrip.asDouble();
    if (deltaT>1.) {
      _lastTrip = clk;
      printf("%s - %s %d pixels %s at event %d, "
             "no trips in last %f seconds, attempt to trip, "
             "shunt, or ignore beam\n",
             _pname,
             _name.c_str(),
             pixelCount,
             _uses_bh ? "in blackhole" : "over threshold",
             _nevt,
             deltaT);
      if(_handler->enabled()) {
        _handler->trip(pixelCount);
      } else {
        printf("%s trip disabled for detector %s, "
               "not closing shutter!\n",
               _pname, _name.c_str());
      }
    }
  }

  _nevt++;
}

CsPadProtector::CsPadProtector(const Pds::DetInfo& info,
                               const Pds::TypeId&  type,
                               void*               payload,
                               const PVHandler*    handler) :
  Protector(CspadName, info, handler, true),
  _bh(NULL),
  _config(NULL),
  _config_size(0),
  _data(NULL)
{
   _bh = new BlackHole(Pds::CsPad::ColumnsPerASIC,
                       Pds::CsPad::MaxRowsPerASIC*2,
                       12000,8,200,1);
  if (type.version() == 5) {
    // need to copy the configuration, since it doesn't persist
    const Pds::CsPad::ConfigV5* config = reinterpret_cast<const Pds::CsPad::ConfigV5*>(payload);
    if (config->_sizeof() > _config_size) {
      // need to allocate a larger buffer
      if (_config) {
        delete[] (char *)(_config);
      }
      char* buffer = new char[config->_sizeof()];
      _config = reinterpret_cast<Pds::CsPad::ConfigV5*>(buffer);
    }
    _config_size = config->_sizeof();
    memcpy(_config, config, config->_sizeof());
  } else {
    printf("%s unsupported Cspad config version for %s: %u\n",
           _pname, _name.c_str(), type.version());
  }
}

CsPadProtector::~CsPadProtector()
{
  if (_bh) {
    delete _bh;
  }

  if (_config) {
    delete[] (char *)(_config);
  }
}

void CsPadProtector::event(const Pds::TypeId& type,
                           void* payload)
{
  if (type.id() == Pds::TypeId::Id_CspadElement) {
    if (type.version() == 2) {
      _data = reinterpret_cast<const Pds::CsPad::DataV2*>(payload);
    } else {
      printf("%s unsupported Cspad data version for %s: %u\n",
             _pname, _name.c_str(), type.version());     
    }
  }
}


bool CsPadProtector::analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount)
{
  bool trip = false;

  for (uint quad=0; quad<_config->numQuads(); quad++) {
    const Pds::CsPad::ElementV2& s = _data->quads(*_config, quad);
    const Pds::CsPad::ConfigV5& cfg = *_config;

    ndarray<const int16_t, 3> data = s.data(cfg);
    if (_nevt==0) {
      _bh->setupGoodPix(data,s.quad(),0);
    } else if (_nevt==1) {
      _bh->updateGoodPix(data,s.quad(),0);
    } else {
      trip =_bh->floodFill(data,s.quad(),0);
      if (trip) {
        pixelCount = _bh->lasttrip_pixcount();
        break;
      }
    }
  }

  _data=0;

  return trip;
}

bool CsPadProtector::ready() const
{
  return _config && _data;
}

template<class Cfg, class Data>
JungfrauProtector<Cfg, Data>::JungfrauProtector(const Pds::DetInfo& info,
                                                const Pds::TypeId&  type,
                                                void*               payload,
                                                const PVHandler*    handler) :
  Protector(JungfrauName, info, handler),
  _config(NULL),
  _config_size(0),
  _data(NULL)
{
  if (type.version() == Cfg::Version) {
    // need to copy the configuration, since it doesn't persist
    const Cfg* config = reinterpret_cast<const Cfg*>(payload);
    if (config->_sizeof() > _config_size) {
      // need to allocate a larger buffer
      if (_config) {
        delete[] (char *)(_config);
      }
      char* buffer = new char[config->_sizeof()];
      _config = reinterpret_cast<Cfg*>(buffer);
    }
    _config_size = config->_sizeof();
    memcpy(_config, config, config->_sizeof());
  } else {
    printf("%s unsupported Jungfrau config version for %s: %u\n",
           _pname, _name.c_str(), type.version());
  }
}

template<class Cfg, class Data>
JungfrauProtector<Cfg, Data>::~JungfrauProtector()
{
  if (_config) {
    delete[] (char *)(_config);
  }
}

template<class Cfg, class Data>
void JungfrauProtector<Cfg, Data>::event(const Pds::TypeId& type,
                                         void* payload)
{
  if (type.id() == Pds::TypeId::Id_JungfrauElement) {
    if (type.version() == Data::Version) {
      _data = reinterpret_cast<const Data*>(payload);
    } else {
      printf("%s unsupported Jungfrau data version for %s: %u\n",
             _pname, _name.c_str(), type.version());     
    }
  }
}

template<class Cfg, class Data>
bool JungfrauProtector<Cfg, Data>::analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount)
{
  bool trip = false;
  static const uint16_t gain_bits = 3<<14;
  static const uint16_t data_bits = (1<<14) - 1;

  const Data& s = *_data;
  const Cfg& cfg = *_config;

  ndarray<const uint16_t, 3> data = s.frame(cfg);
  for (unsigned i=0; i<data.shape()[0]; i++) {
    for (unsigned j=0; j<data.shape()[1]; j++) {
      for (unsigned k=0; k<data.shape()[2]; k++) {
        uint16_t value = data(i,j,k);
        if (((value&gain_bits) == gain_bits) &&
            ((value&data_bits) > _handler->threshold())) {
          if(++pixelCount > _handler->npixels()) {
            trip = true;
          }
        }   
      }
    }
  }

  _data = 0;

  return trip;
}

template<class Cfg, class Data>
bool JungfrauProtector<Cfg, Data>::ready() const
{
  return _config && _data;
}

template<class Cfg, class Data>
const unsigned EpixArrayProtector<Cfg, Data>::nE = Cfg::_numberOfElements;

template<class Cfg, class Data>
const unsigned EpixArrayProtector<Cfg, Data>::wE = EpixCfg10ka::_numberOfPixelsPerAsicRow * EpixCfg10ka::_numberOfAsicsPerRow;

template<class Cfg, class Data>
const unsigned EpixArrayProtector<Cfg, Data>::hE = EpixCfg10ka::_numberOfRowsPerAsic * EpixCfg10ka::_numberOfAsicsPerColumn;

template<class Cfg, class Data>
const unsigned EpixArrayProtector<Cfg, Data>::asicMap[] = { 2, 1, 3, 0 };

template<class Cfg, class Data>
EpixArrayProtector<Cfg, Data>::EpixArrayProtector(const Pds::DetInfo& info,
                                                  const Pds::TypeId&  type,
                                                  void*               payload,
                                                  const PVHandler*    handler) :
  Protector(EpixArrayName, info, handler),
  _config(NULL),
  _config_size(0),
  _data(NULL),
  _gain_mask(0)
{
  if (type.version() == Cfg::Version) {
    // need to copy the configuration, since it doesn't persist
    const Cfg* config = reinterpret_cast<const Cfg*>(payload);
    if (config->_sizeof() > _config_size) {
      // need to allocate a larger buffer
      if (_config) {
        delete[] (char *)(_config);
      }
      char* buffer = new char[config->_sizeof()];
      _config = reinterpret_cast<Cfg*>(buffer);
    }
    _config_size = config->_sizeof();
    memcpy(_config, config, config->_sizeof());
    // fill the gain config info
    unsigned ngains[] = {0, 0, 0};
    _gain_cfg = make_ndarray<uint16_t>(nE, hE, wE);
    for(unsigned i=0; i<nE; i++) {
      const EpixCfg10ka& eC = _config->elemCfg(i);
      ndarray<const uint16_t,2> asicPixelConfig = eC.asicPixelConfigArray();
      for(unsigned j=0; j<hE; j++) {
        for(unsigned k=0; k<wE; k++) {
          uint16_t gain_config = 0;
          uint16_t gain_bits = (asicPixelConfig(j,k) & conf_bits) |
                               (eC.asics(asicMap[(j/EpixCfg10ka::_numberOfRowsPerAsic)*EpixCfg10ka::_numberOfAsicsPerRow +
                                                 k/EpixCfg10ka::_numberOfPixelsPerAsicRow]).trbit() << 4);
          switch(gain_bits) {
            case FH:
              gain_config = 0;
              ngains[0]++;
              break;
            case FM:
              gain_config = 1;
              ngains[1]++;
              break;
            case FL:
            case FL_ALT:
              gain_config = 2;
              ngains[2]++;
              break;
            case AHL:
            case AHL_FORCE:
              gain_config = 3;
              ngains[2]++;
              break;
            case AML:
            case AML_FORCE:
              gain_config = 4;
              ngains[2]++;
              break;
            default:
              gain_config = 0;
              break;
          }
          _gain_cfg(i,j,k) = gain_config;
        }
      }
    }
    // pick which gain modes can trip
    if (ngains[2] > 0) {
      // only fixed low or both switched low
      _gain_mask = 0x64;
    } else if (ngains[1] > 0) {
      // only fixed med gain
      _gain_mask = 0x2;
    } else {
      // only fixed high gain
      _gain_mask = 0x1;
    }
  } else {
    printf("%s unsupported EpixArray config version for %s: %u\n",
           _pname, _name.c_str(), type.version());
  }
}

template<class Cfg, class Data>
EpixArrayProtector<Cfg, Data>::~EpixArrayProtector()
{
  if (_config) {
    delete[] (char *)(_config);
  }
}

template<class Cfg, class Data>
void EpixArrayProtector<Cfg, Data>::event(const Pds::TypeId& type,
                                          void* payload)
{
  if (type.id() == Pds::TypeId::Id_Epix10kaArray) {
    if (type.version() == Data::Version) {
      _data = reinterpret_cast<const Data*>(payload);
    } else {
      printf("%s unsupported EpixArray data version for %s: %u\n",
             _pname, _name.c_str(), type.version());     
    }
  }
}

template<class Cfg, class Data>
bool EpixArrayProtector<Cfg, Data>::analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount)
{
  bool trip = false;
  static const uint16_t gain_bits = 3<<14;
  static const uint16_t data_bits = (1<<14) - 1;

  const Data& s = *_data;
  const Cfg& cfg = *_config;

  ndarray<const uint16_t, 3> data = s.frame(cfg);
  for (unsigned i=0; i<data.shape()[0]; i++) {
    for (unsigned j=0; j<data.shape()[1]; j++) {
      for (unsigned k=0; k<data.shape()[2]; k++) {
        uint16_t value = data(i,j,k);
        uint16_t gain_mode = _gain_cfg(i,j,k);
        // keep if switched
        if ((gain_mode > 2) && (value & gain_bits))
          gain_mode += 2;
        if (((1<<gain_mode) & _gain_mask) &&
            ((value & data_bits) > _handler->threshold())){
          if(++pixelCount > _handler->npixels()) {
            trip = true;
          }
        }
      }
    }
  }

  _data = 0;

  return trip;
}

template<class Cfg, class Data>
bool EpixArrayProtector<Cfg, Data>::ready() const
{
  return _config && _data;
}

template<class Cfg, class Data>
const unsigned Epix10kaProtector<Cfg, Data>::asicMap[] = { 2, 1, 3, 0 };

template<class Cfg, class Data>
Epix10kaProtector<Cfg, Data>::Epix10kaProtector(const Pds::DetInfo& info,
                                                const Pds::TypeId&  type,
                                                void*               payload,
                                                const PVHandler*    handler) :
  Protector(Epix10kaName, info, handler),
  _config(NULL),
  _config_size(0),
  _data(NULL),
  _gain_mask(0)
{
  if (type.version() == Cfg::Version) {
    // need to copy the configuration, since it doesn't persist
    const Cfg* config = reinterpret_cast<const Cfg*>(payload);
    if (config->_sizeof() > _config_size) {
      // need to allocate a larger buffer
      if (_config) {
        delete[] (char *)(_config);
      }
      char* buffer = new char[config->_sizeof()];
      _config = reinterpret_cast<Cfg*>(buffer);
    }
    _config_size = config->_sizeof();
    memcpy(_config, config, config->_sizeof());
    // fill the gain config info
    unsigned ngains[] = {0, 0, 0};
    unsigned wE = _config->numberOfRows();
    unsigned hE = _config->numberOfColumns();
    _gain_cfg = make_ndarray<uint16_t>(hE, wE);
    ndarray<const uint16_t,2> asicPixelConfig = _config->asicPixelConfigArray();
    for(unsigned j=0; j<hE; j++) {
      for(unsigned k=0; k<wE; k++) {
        uint16_t gain_config = 0;
        uint16_t gain_bits = (asicPixelConfig(j,k) & conf_bits) |
                             (_config->asics(asicMap[(j/_config->numberOfRowsPerAsic())*_config->numberOfAsicsPerRow() +
                                                     k/_config->numberOfPixelsPerAsicRow()]).trbit() << 4);
        switch(gain_bits) {
          case FH:
            gain_config = 0;
            ngains[0]++;
            break;
          case FM:
            gain_config = 1;
            ngains[1]++;
            break;
          case FL:
          case FL_ALT:
            gain_config = 2;
            ngains[2]++;
            break;
          case AHL:
          case AHL_FORCE:
            gain_config = 3;
            ngains[2]++;
            break;
          case AML:
          case AML_FORCE:
            gain_config = 4;
            ngains[2]++;
            break;
          default:
            gain_config = 0;
            break;
          }
          _gain_cfg(j,k) = gain_config;
      }
    }
    // pick which gain modes can trip
    if (ngains[2] > 0) {
      // only fixed low or both switched low
      _gain_mask = 0x64;
    } else if (ngains[1] > 0) {
      // only fixed med gain
      _gain_mask = 0x2;
    } else {
      // only fixed high gain
      _gain_mask = 0x1;
    }
  } else {
    printf("%s unsupported Epix10ka config version for %s: %u\n",
           _pname, _name.c_str(), type.version());
  }
}

template<class Cfg, class Data>
Epix10kaProtector<Cfg, Data>::~Epix10kaProtector()
{
  if (_config) {
    delete[] (char *)(_config);
  }
}

template<class Cfg, class Data>
void Epix10kaProtector<Cfg, Data>::event(const Pds::TypeId& type,
                                         void* payload)
{
  if (type.id() == Pds::TypeId::Id_EpixElement) {
    if (type.version() == Data::Version) {
      _data = reinterpret_cast<const Data*>(payload);
    } else {
      printf("%s unsupported Epix10ka data version for %s: %u\n",
             _pname, _name.c_str(), type.version());
    }
  }
}

template<class Cfg, class Data>
bool Epix10kaProtector<Cfg, Data>::analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount)
{
  bool trip = false;
  static const uint16_t gain_bits = 3<<14;
  static const uint16_t data_bits = (1<<14) - 1;

  const Data& s = *_data;
  const Cfg& cfg = *_config;

  ndarray<const uint16_t, 2> data = s.frame(cfg);
  for (unsigned j=0; j<data.shape()[0]; j++) {
    for (unsigned k=0; k<data.shape()[1]; k++) {
      uint16_t value = data(j,k);
      uint16_t gain_mode = _gain_cfg(j,k);
      // keep if switched
      if ((gain_mode > 2) && (value & gain_bits))
        gain_mode += 2;
      if (((1<<gain_mode) & _gain_mask) &&
          ((value & data_bits) > _handler->threshold())){
        if(++pixelCount > _handler->npixels()) {
          trip = true;
        }
      }
    }
  }

  _data = 0;

  return trip;
}

template<class Cfg, class Data>
bool Epix10kaProtector<Cfg, Data>::ready() const
{
  return _config && _data;
}

template<class Cfg, class Data>
EpixProtector<Cfg, Data>::EpixProtector(const Pds::DetInfo& info,
                                        const Pds::TypeId&  type,
                                        void*               payload,
                                        const PVHandler*    handler) :
  Protector(EpixName, info, handler),
  _config(NULL),
  _config_size(0),
  _data(NULL)
{
  if (type.version() == Cfg::Version) {
    // need to copy the configuration, since it doesn't persist
    const Cfg* config = reinterpret_cast<const Cfg*>(payload);
    if (config->_sizeof() > _config_size) {
      // need to allocate a larger buffer
      if (_config) {
        delete[] (char *)(_config);
      }
      char* buffer = new char[config->_sizeof()];
      _config = reinterpret_cast<Cfg*>(buffer);
    }
    _config_size = config->_sizeof();
    memcpy(_config, config, config->_sizeof());
  } else {
    printf("%s unsupported Epix config version for %s: %u\n",
           _pname, _name.c_str(), type.version());
  }
}

template<class Cfg, class Data>
EpixProtector<Cfg, Data>::~EpixProtector()
{
  if (_config) {
    delete[] (char *)(_config);
  }
}

template<class Cfg, class Data>
void EpixProtector<Cfg, Data>::event(const Pds::TypeId& type,
                                     void* payload)
{
  if (type.id() == Pds::TypeId::Id_EpixElement) {
    if (type.version() == Data::Version) {
      _data = reinterpret_cast<const Data*>(payload);
    } else {
      printf("%s unsupported Epix data version for %s: %u\n",
             _pname, _name.c_str(), type.version());
    }
  }
}

template<class Cfg, class Data>
bool EpixProtector<Cfg, Data>::analyzeDetector(const Pds::ClockTime& clk, int32_t& pixelCount)
{
  bool trip = false;

  const Data& s = *_data;
  const Cfg& cfg = *_config;

  ndarray<const uint16_t, 2> data = s.frame(cfg);
  for (unsigned i=0; i<data.shape()[0]; i++) {
    for (unsigned j=0; j<data.shape()[1]; j++) {
      if (data(i,j) > _handler->threshold()) {
        if (++pixelCount > _handler->npixels()) {
          trip = true;
        }
      }
    }
  }

  _data = 0;

  return trip;
}

template<class Cfg, class Data>
bool EpixProtector<Cfg, Data>::ready() const
{
  return _config && _data;
}
