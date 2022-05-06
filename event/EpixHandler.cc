#include "EpixHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/event/GainSwitchCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/ImageMask.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/genericpgp.ddl.h"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/xtc/ClockTime.hh"

#include <string.h>
#include <string>
#include <sstream>

#define DBUG

using namespace Ami;

static const unsigned offset=1<<16;
static const double  doffset=double(offset);
static const unsigned gain_bits = 3<<14;
static const unsigned data_bits = ((1<<16) - 1) - gain_bits;
static const unsigned conf_bits = 0x1c;
static const double  RDIV=30000;
static const double prec_norm_factor=100.0;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_EpixConfig);
  types.push_back(Pds::TypeId::Id_Epix10kConfig);
  types.push_back(Pds::TypeId::Id_Epix10kaConfig);
  types.push_back(Pds::TypeId::Id_Epix100aConfig);
  types.push_back(Pds::TypeId::Id_GenericPgpConfig);
  return types;
}

static const Pds::TypeId Data_Type   = Pds::TypeId(Pds::TypeId::Id_EpixElement,1);

static const unsigned _channel_map[] = { 11, 10, 9,  8,
                                         7,  6,  5,  4,
                                         12, 13, 14, 15,
                                         0,  1,  2,  3 };

//                        2|1
//  ASICs are numbered    -+-
//                        3|0
static const unsigned _asic_map[] = { 2, 1, 3, 0 };
struct AsicLocation { unsigned row, col; };
typedef struct AsicLocation AsicLocation_s;
static const AsicLocation_s _asicLocation[] = { {1,1}, {0,1}, {0,0}, {1,0} };

static void _load_one_asic(ndarray<unsigned,2>& asic_array,
                           unsigned asic,
                           ndarray<unsigned,2>& full_array);

static void _load_one_asic(ndarray<double,3>& asic_array,
                           unsigned asic,
                           ndarray<double,3>& full_array);

static double _tps_temp(const uint16_t q);

static CspadTemp           _therm(RDIV);

namespace EpixAmi {
  class MapElement {
  public:
    unsigned id;  // unique identifier / serial number
    int      x;   // x-location of first pixel
    int      y;   // y-location of first pixel
    unsigned orientation; // orientation of pixels
  };

  class Geometry {
  public:
    Geometry() {}
    Geometry(const Pds::DetInfo&,
	     const Pds::Epix::ConfigV1&);
  public:
    unsigned columns() const;
    unsigned rows   () const;
  };

  class EnvData {
  public:
    virtual ~EnvData() {}
    virtual void     addFeatures(const char*   name)=0;
    virtual void     rename     (const char*   name)=0;
    virtual void     fill       (FeatureCache& cache,
                                 ndarray<const uint16_t,2> env) = 0;
    virtual void     fill       (FeatureCache& cache,
                                 ndarray<const uint32_t,2> env) = 0;
  };

  class EnvData1 : public EnvData {
  public:
    EnvData1(EventHandlerF& h);
  public:
    void     addFeatures(const char* name);
    void     rename     (const char* name);
    void     fill       (FeatureCache& cache,
                         ndarray<const uint16_t,2> env);
    void     fill       (FeatureCache& cache,
                         ndarray<const uint32_t,2> env) {}
  private:
    EventHandlerF& _handler;
    ndarray<int,1> _index;
  };

  class EnvData2 : public EnvData {
  public:
    EnvData2(EventHandlerF& h);
  public:
    void     addFeatures(const char* name);
    void     rename     (const char* name);
    void     fill       (FeatureCache& cache,
                         ndarray<const uint16_t,2> env) {}
    void     fill       (FeatureCache& cache,
                         ndarray<const uint32_t,2> env);
  private:
    EventHandlerF& _handler;
    ndarray<int,1> _index;
  };

  class ConfigCache {
  public:
    ConfigCache();
    ConfigCache(const ConfigCache&);
    ConfigCache(Pds::TypeId, const void*, EventHandlerF*);
    ~ConfigCache();
  public:
    unsigned numberOfAsics   () const  { return _nAsics; }
    unsigned asicMask        () const { return _aMask; }
    EnvData* envData         () const { return _envData; }
  public:
    // ASIC layout
    unsigned numberOfAsicsPerRow   () const { return _nchip_columns; }
    unsigned numberOfAsicsPerColumn() const { return _nchip_rows; }

    // Full size
    unsigned numberOfColumns() const { return _columns; }
    unsigned numberOfRows   () const { return _rows; }
    unsigned numberOfRowsCal() const { return _rowsCal; }

    // event data size
    unsigned numberOfRowsRead() const { return _rowsRead; }

    // subframe size
    unsigned numberOfColumnsPerAsic () const { return _colsPerAsic; }
    unsigned numberOfRowsPerAsic    () const { return _rowsPerAsic; }
    unsigned numberOfRowsCalPerAsic () const { return _rowsCalPerAsic; }

    // subframe data size
    unsigned numberOfRowsReadPerAsic() const { return _rowsReadPerAsic; }

    // number of gain modes
    unsigned numberOfGainModes     () const { return _nGainModes; }
    unsigned numberOfFixedGainModes() const { return _nFixedGainModes; }

    // asic pixel config
    ndarray <const uint16_t,2> pixelGainConfig() const { return _pixelGainConfig; }
  public:
    void event(const void* payload,
               ndarray<const uint16_t,2>& frame,
               ndarray<const uint16_t,1>& temps,
               ndarray<const uint16_t,2>& cal,
               FeatureCache&              cache) {
#define PARSE_CONFIG(typ,dtyp,etyp)                                       \
      const dtyp& f = *reinterpret_cast<const dtyp*>(payload);            \
      const typ& c = *reinterpret_cast<const typ*>(_buffer);              \
      frame = f.frame(c);                                                 \
      temps = f.temperatures(c);                                          \
      _envData->fill(cache,f.etyp(c));

      switch(_id.id()) {
      case Pds::TypeId::Id_EpixConfig   :
        { PARSE_CONFIG(Pds::Epix::ConfigV1    ,Pds::Epix::ElementV1, excludedRows) }
        break;
      case Pds::TypeId::Id_Epix10kConfig:
        { PARSE_CONFIG(Pds::Epix::Config10KV1 ,Pds::Epix::ElementV1, excludedRows); }
        break;
      case Pds::TypeId::Id_Epix10kaConfig:
        switch (_id.version()) {
        case 1:
          { PARSE_CONFIG(Pds::Epix::Config10kaV1 ,Pds::Epix::ElementV3, environmentalRows);
            cal = f.calibrationRows(c); } break;
        case 2:
          { PARSE_CONFIG(Pds::Epix::Config10kaV2 ,Pds::Epix::ElementV3, environmentalRows);
            cal = f.calibrationRows(c); } break;
        default:
          break;
        }
        break;
      case Pds::TypeId::Id_Epix100aConfig:
        switch (_id.version()) {
        case 1:
          { PARSE_CONFIG(Pds::Epix::Config100aV1,Pds::Epix::ElementV2, environmentalRows);
            cal = f.calibrationRows(c); } break;
        case 2:
          { PARSE_CONFIG(Pds::Epix::Config100aV2,Pds::Epix::ElementV3, environmentalRows);
            cal = f.calibrationRows(c); } break;
        default:
          break;
        }
        break;
      default:
        break;
      }
#undef PARSE_CONFIG
    }
    void dump() const;
  private:
    static const unsigned MAX_PRINT = 100;
    enum { AML=0, AML_FORCE=4, FL=8, FM=12, AHL=16, AHL_FORCE=20, FL_ALT=24, FH=28 };
    Pds::TypeId _id;
    char*       _buffer;

    unsigned _nAsics;
    unsigned _aMask;
    unsigned _nchip_columns;
    unsigned _nchip_rows;
    unsigned _columns;
    unsigned _colsPerAsic;
    unsigned _rows;
    unsigned _rowsPerAsic;
    unsigned _rowsRead;
    unsigned _rowsReadPerAsic;
    unsigned _rowsCal;
    unsigned _rowsCalPerAsic;
    unsigned _nGainModes;
    unsigned _nFixedGainModes;
    EnvData* _envData;
    ndarray<uint16_t,2> _pixelGainConfig;
  };
};

EpixAmi::ConfigCache::ConfigCache() :
  _id    (Pds::TypeId(Pds::TypeId::Any,0)),
  _buffer(0)
{
}

EpixAmi::ConfigCache::ConfigCache(Pds::TypeId tid, const void* payload,
                                  EventHandlerF* handler) : _id(tid)
{
  _nchip_columns=1;
  _nchip_rows   =1;
  _columns = 0;
  _rows    = _rowsRead = _rowsCal = 0;
  _rowsPerAsic = _rowsReadPerAsic = _rowsCalPerAsic = 0;
  _colsPerAsic = 0;
  _aMask = 0;
  _nAsics = 0;
  _nGainModes = 1;
  _nFixedGainModes = 1;
  _pixelGainConfig = make_ndarray<uint16_t>(_rows, _columns);

  if (tid.id()==Pds::TypeId::Id_GenericPgpConfig) {
    const Pds::GenericPgp::ConfigV1& c = *reinterpret_cast<const Pds::GenericPgp::ConfigV1*>(payload);
    reinterpret_cast<uint32_t&>(tid) = c.stream()[0].config_type();
    payload = reinterpret_cast<const void*>(c.payload().data()+c.stream()[0].config_offset());
  }

#define PARSE_CONFIG(typ)                                               \
  const typ& c = *reinterpret_cast<const typ*>(payload);                \
  _buffer = new char[c._sizeof()];                                      \
  memcpy(_buffer, &c, c._sizeof());                                     \
  _nchip_columns=c.numberOfAsicsPerRow();                               \
  _nchip_rows   =c.numberOfAsicsPerColumn();                            \
  _columns = c.numberOfColumns();                                       \
  _colsPerAsic = c.numberOfPixelsPerAsicRow();                          \
  _rows = c.numberOfRows();                                             \
  _rowsPerAsic = c.numberOfRowsPerAsic();                               \
  _aMask  = c.asicMask() ? c.asicMask() : 0xf;                          \
  _nAsics = c.numberOfAsics();                                          \
  _envData = ((c.version()>>16)&0xf) < 2 ? (EnvData*)new EnvData1(*handler) : (EnvData*)new EnvData2(*handler);

  switch(_id.id()) {
  case Pds::TypeId::Id_EpixConfig   :
    { PARSE_CONFIG(Pds::Epix::ConfigV1);
      _pixelGainConfig = make_ndarray<uint16_t>(_rows, _columns);
      for(uint16_t* val = _pixelGainConfig.begin(); val != _pixelGainConfig.end(); val++) {
        *val = 0;
      }
      _nGainModes      = 1;
      _nFixedGainModes = _nGainModes;
      _rowsRead        = _rows;
      _rowsReadPerAsic = _rows/_nchip_rows; } break;
  case Pds::TypeId::Id_Epix10kConfig:
    { PARSE_CONFIG(Pds::Epix::Config10KV1);
      _pixelGainConfig = make_ndarray<uint16_t>(_rows, _columns);
      for(uint16_t* val = _pixelGainConfig.begin(); val != _pixelGainConfig.end(); val++) {
        *val = 0;
      }
      _nGainModes      = 1;
      _nFixedGainModes = _nGainModes;
      _rowsRead        = _rows;
      _rowsReadPerAsic = _rows/_nchip_rows; } break;
  case Pds::TypeId::Id_Epix10kaConfig:
    switch(_id.version()) {
    case 1:
      { PARSE_CONFIG(Pds::Epix::Config10kaV1);
        ndarray<const uint16_t,2> asicPixelConfig = c.asicPixelConfigArray();
        _pixelGainConfig = make_ndarray<uint16_t>(_rows, _columns);
        unsigned nprint = 0;
        for(unsigned j=0; j<_rows; j++) {
          for(unsigned k=0; k<_columns; k++) {
            uint16_t gain_config = 0;
            uint16_t gain_bits = (asicPixelConfig(j,k) & conf_bits) |
                                 (c.asics(_asic_map[(j/_rowsPerAsic)*_nchip_columns + k/_colsPerAsic]).trbit() << 4);
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
              case AHL_FORCE:
                gain_config = 3;
                break;
              case AML:
              case AML_FORCE:
                gain_config = 4;
                break;
              default:
                if (nprint < MAX_PRINT) {
                  printf("EpixHandler::event unknown gain control bits %x for pixel (%u, %u)\n", gain_bits, j, k);
                  nprint++;
                }
                gain_config = 0;
                break;
            }
            _pixelGainConfig(j,k) = gain_config;
          }
        }
        _nGainModes      = 7;
        _nFixedGainModes = 3;
        _rowsRead        = _rows;
        _rowsReadPerAsic = _rows/_nchip_rows;
        _rowsCal         = c.numberOfCalibrationRows();
        _rowsCalPerAsic  = _rowsCal/c.numberOfAsicsPerColumn(); } break;
    case 2:
      { PARSE_CONFIG(Pds::Epix::Config10kaV2);
        ndarray<const uint16_t,2> asicPixelConfig = c.asicPixelConfigArray();
        _pixelGainConfig = make_ndarray<uint16_t>(_rows, _columns);
        unsigned nprint = 0;
        for(unsigned j=0; j<_rows; j++) {
          for(unsigned k=0; k<_columns; k++) {
            uint16_t gain_config = 0;
            uint16_t gain_bits = (asicPixelConfig(j,k) & conf_bits) |
                                 (c.asics(_asic_map[(j/_rowsPerAsic)*_nchip_columns + k/_colsPerAsic]).trbit() << 4);
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
              case AHL_FORCE:
                gain_config = 3;
                break;
              case AML:
              case AML_FORCE:
                gain_config = 4;
                break;
              default:
                if (nprint < MAX_PRINT) {
                  printf("EpixHandler::event unknown gain control bits %x for pixel (%u, %u)\n", gain_bits, j, k);
                  nprint++;
                }
                gain_config = 0;
                break;
            }
            _pixelGainConfig(j,k) = gain_config;
          }
        }
        _nGainModes      = 7;
        _nFixedGainModes = 3;
        _rowsRead        = _rows;
        _rowsReadPerAsic = _rows/_nchip_rows;
        _rowsCal         = c.numberOfCalibrationRows();
        _rowsCalPerAsic  = _rowsCal/c.numberOfAsicsPerColumn(); } break;

    default:
      printf("Epix10kaConfig version %d unknown\n",_id.version());
      break;
    }
    break;
  case Pds::TypeId::Id_Epix100aConfig:
    switch(_id.version()) {
    case 1:
      { PARSE_CONFIG(Pds::Epix::Config100aV1);
        _pixelGainConfig = make_ndarray<uint16_t>(_rows, _columns);
        for(uint16_t* val = _pixelGainConfig.begin(); val != _pixelGainConfig.end(); val++) {
          *val = 0;
        }
        _nGainModes      = 1;
        _nFixedGainModes = _nGainModes;
        _rowsRead        = c.numberOfReadableRows();
        _rowsReadPerAsic = c.numberOfReadableRowsPerAsic();
        _rowsCal         = c.numberOfCalibrationRows();
        _rowsCalPerAsic  = _rowsCal/c.numberOfAsicsPerColumn(); } break;
    case 2:
      { PARSE_CONFIG(Pds::Epix::Config100aV2);
        _pixelGainConfig = make_ndarray<uint16_t>(_rows, _columns);
        for(uint16_t* val = _pixelGainConfig.begin(); val != _pixelGainConfig.end(); val++) {
          *val = 0;
        }
        _nGainModes      = 1;
        _nFixedGainModes = _nGainModes;
        _rowsRead        = c.numberOfReadableRows();
        _rowsReadPerAsic = c.numberOfReadableRowsPerAsic();
        _rowsCal         = c.numberOfCalibrationRows();
        _rowsCalPerAsic  = _rowsCal/c.numberOfAsicsPerColumn(); } break;
    default:
      printf("Epix100aConfig version %d unknown\n",_id.version());
      break;
    }
    break;
  default:
    printf("EpixHandler::ConfigCache unrecognized configuration type %08x\n",
           _id.value());
    return;
  }
#undef PARSE_CONFIG
}

EpixAmi::ConfigCache::ConfigCache(const EpixAmi::ConfigCache& o) :
  _id             (o._id),
  _nAsics         (o._nAsics),
  _aMask          (o._aMask),
  _nchip_columns  (o._nchip_columns),
  _nchip_rows     (o._nchip_rows),
  _columns        (o._columns),
  _colsPerAsic    (o._colsPerAsic),
  _rows           (o._rows),
  _rowsPerAsic    (o._rowsPerAsic),
  _rowsRead       (o._rowsRead),
  _rowsReadPerAsic(o._rowsReadPerAsic),
  _rowsCal        (o._rowsCal),
  _rowsCalPerAsic (o._rowsCalPerAsic),
  _nGainModes     (o._nGainModes),
  _nFixedGainModes(o._nFixedGainModes),
  _pixelGainConfig(o._pixelGainConfig)
{
  unsigned sz=0;
#define PARSE_CONFIG(typ)                                     \
  const typ& c = *reinterpret_cast<const typ*>(o._buffer);    \
  sz = c._sizeof();

  switch(_id.id()) {
  case Pds::TypeId::Id_EpixConfig   :
    { PARSE_CONFIG(Pds::Epix::ConfigV1) } break;
  case Pds::TypeId::Id_Epix10kConfig:
    { PARSE_CONFIG(Pds::Epix::Config10KV1) } break;
  case Pds::TypeId::Id_Epix10kaConfig:
    switch(_id.version()) {
    case 1:
      { PARSE_CONFIG(Pds::Epix::Config10kaV1) } break;
    case 2:
      { PARSE_CONFIG(Pds::Epix::Config10kaV2) } break;
    default: break;
    }
    break;
  case Pds::TypeId::Id_Epix100aConfig:
    switch(_id.version()) {
    case 1:
      { PARSE_CONFIG(Pds::Epix::Config100aV1) } break;
    case 2:
      { PARSE_CONFIG(Pds::Epix::Config100aV2) } break;
    default: break;
    }
    break;
  default: break;
  }
  if (sz) {
    _buffer = new char[sz];
    memcpy(_buffer, o._buffer, sz);
  }
  else
    _buffer = 0;
#undef PARSE_CONFIG
}

EpixAmi::ConfigCache::~ConfigCache()
{
  if (_buffer) delete[] _buffer;
}

void EpixAmi::ConfigCache::dump() const
{
  printf("typeid %08x  buffer %p\n",_id.value(),_buffer);
  printf("nchip_cols %u  rows %u\n",_nchip_columns,_nchip_rows);
  printf("columns    %u  perasic %u\n",_columns,_colsPerAsic);
  printf("rows       %u  perasic %u\n",_rows,_rowsPerAsic);
  printf("rowsRead   %u  perasic %u\n",_rowsRead,_rowsReadPerAsic);
  printf("rowsCal    %u  perasic %u\n",_rowsCal ,_rowsCalPerAsic);
}

EpixHandler::EpixHandler(const Pds::DetInfo& info, FeatureCache& cache) :
  EventHandlerF  (info, Data_Type.id(), config_type_list(), cache),
  _cache        (cache),
  _desc         ("template",0,0),
  _entry        (0),
  _ref          (0),
  _options      (0),
  _do_norm      (false),
  _status       (make_ndarray<unsigned>(0U,0)),
  _pedestals    (make_ndarray<double>(0U,0,0)),
  _gains        (make_ndarray<double>(0U,0,0)),
  _config_cache (new EpixAmi::ConfigCache)
{
}

EpixHandler::~EpixHandler()
{
  delete _config_cache;
  reset();
}

unsigned EpixHandler::nentries() const { return (_entry ? 1 : 0) + (_ref ? 1 : 0); }

const Entry* EpixHandler::entry(unsigned i) const {
  switch(i) {
  case 0: return _entry;
  case 1: return _ref;
  default: return 0; }
}

void EpixHandler::rename(const char* s)
{
  if (_entry) {
    _entry->desc().name(s);

    if (_ref) {
      std::ostringstream ostr;
      ostr << s << "-Cal";
      _ref->desc().name(ostr.str().c_str());
    }

    int index=0;
    unsigned nAsics       =_config_cache->numberOfAsics();
    for(unsigned a=0; a<nAsics; a++) {
      std::ostringstream ostr;
      ostr << s << ":AsicMonitor" << a;
      _rename_cache(_feature[index++],ostr.str().c_str());
    }

    EpixAmi::EnvData* envData =_config_cache->envData();
    if (envData)
      envData->rename(s);

    if (Ami::EventHandler::post_diagnostics())
      for(unsigned a=0; a<16; a++) {
        std::ostringstream ostr;
        ostr << s << ":CommonMode" << _channel_map[a];
        _rename_cache(_feature[index++],ostr.str().c_str());
      }
  }
}

void EpixHandler::reset()
{
  _entry = 0;
  _ref   = 0;

  for(unsigned i=0; i<_ewf.size(); i++)
    delete _ewf[i];
  _ewf.clear();
}

//
//

void EpixHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  const char* detname = Pds::DetInfo::name(det.detector());

  delete _config_cache;
  _config_cache = new EpixAmi::ConfigCache(tid,payload,this);
  _config_cache->dump();

  unsigned rows    = _config_cache->numberOfRowsRead();
  unsigned columns = _config_cache->numberOfColumns();
  unsigned nchip_rows    = _config_cache->numberOfAsicsPerColumn();
  unsigned nchip_columns = _config_cache->numberOfAsicsPerRow();
  unsigned colsPerAsic   = _config_cache->numberOfColumnsPerAsic();
  unsigned rowsPerAsic   = _config_cache->numberOfRowsReadPerAsic();

  //
  //  Special case of one ASIC
  //     Make frame only as large as one ASIC
  //
  unsigned aMask = _config_cache->asicMask();
  if (aMask==0) return;

  if ((aMask&(aMask-1))==0) {
    columns = colsPerAsic;
    rows    = rowsPerAsic;

    int ppb = image_ppbin  (columns,rows);
    int dpb = display_ppbin(columns,rows);
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns, rows, ppb, ppb, dpb, dpb);

    _desc = desc;
    float x0 = 0;
    float y0 = 0;
    float x1 = x0+colsPerAsic;
    float y1 = y0+rowsPerAsic;
    _desc.add_frame(desc.xbin(x0),desc.ybin(y0),
                    desc.xbin(x1)-desc.xbin(x0),
                    desc.ybin(y1)-desc.ybin(y0));

    _entry = new EntryImage(desc);
    _entry->invalid();
  }
  else {
    int ppb = image_ppbin(columns,rows);
    int dpb = display_ppbin(columns,rows);
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns, rows, ppb, ppb, dpb, dpb);

    _desc = desc;
    for(unsigned i=0; i<nchip_rows; i++)
      for(unsigned j=0; j<nchip_columns; j++) {
        float x0 = j*(colsPerAsic);
        float y0 = i*(rowsPerAsic);
        float x1 = x0+colsPerAsic;
        float y1 = y0+rowsPerAsic;
        _desc.add_frame(desc.xbin(x0),desc.ybin(y0),
                        desc.xbin(x1)-desc.xbin(x0),
                        desc.ybin(y1)-desc.ybin(y0));
      }

    _entry = new EntryImage(desc);
    _entry->invalid();
  }

  if (_config_cache->numberOfRowsCal()) {
    _ewf.resize(_config_cache->numberOfRowsCal());
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    unsigned channelNumber=0;
    for(unsigned i=0; i<_config_cache->numberOfRowsCal(); i++) {
      DescWaveform desc(det, channelNumber,
                        ChannelID::name(det,channelNumber),
                        "Column","Value",
                        _config_cache->numberOfColumns(),
                        0., float(_config_cache->numberOfColumns()));
      _ewf[channelNumber++] = new EntryWaveform(desc);
    }
    Channel ch((1<<channelNumber)-1,Channel::BitMask);
    std::string name(ChannelID::name(det,ch));
    name += "-Cal";
    _ref = new EntryRef(DescRef(det,ch,name.c_str()));
    _ref->set(_ewf.data());
    printf("EpixHandler ref %s [%s]\n",name.c_str(),_ref->desc().name());
  }

  unsigned nFeatures = _config_cache->numberOfAsics();

  if (Ami::EventHandler::post_diagnostics())
    nFeatures += 16;
  _feature = make_ndarray<int>(nFeatures);

  int index=0;
  for(unsigned a=0; a<_config_cache->numberOfAsics(); a++) {
    std::ostringstream ostr;
    ostr << detname << ":Epix:AsicMonitor:" << a;
    _feature[index++] = _add_to_cache(ostr.str().c_str());
  }

  if (_config_cache->envData())
    _config_cache->envData()->addFeatures(detname);

  if (Ami::EventHandler::post_diagnostics()) {
    for(unsigned a=0; a<16; a++) {
      std::ostringstream ostr;
      ostr << detname << ":Epix:CommonMode:" << _channel_map[a];
      _feature[index++] = _add_to_cache(ostr.str().c_str());
    }
  }

  _load_pedestals();
  _load_gains    ();
}

void EpixHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void EpixHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("EpixHandler::event options %x -> %x\n", _options, o);
      _options = o;
    }

    if (_options & FrameCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( _options&~FrameCalib::option_reload_pedestal() );
    }

    // Set the normalization if gain correcting
    double norm = 1.0;
    if (!(_options & FrameCalib::option_no_pedestal())) {
      if (_do_norm && (_options & FrameCalib::option_correct_gain())) {
        norm = prec_norm_factor;
      }
    }

    ndarray<const uint16_t,2> a;
    ndarray<const uint16_t,1> temps;
    ndarray<const uint16_t,2> cal;

    _config_cache->event(payload, a, temps, cal, _cache);

    _entry->reset();
    const DescImage& d = _entry->desc();

    int ppbin = _entry->desc().ppxbin();

    //
    //  Special case of one ASIC
    //
    unsigned rows           = _config_cache->numberOfRowsRead();;
    unsigned cols           = _config_cache->numberOfColumns();
    unsigned nchip_rows     = _config_cache->numberOfAsicsPerColumn();
    unsigned nchip_columns  = _config_cache->numberOfAsicsPerRow();
    unsigned colsPerAsic    = _config_cache->numberOfColumnsPerAsic();
    unsigned rowsPerAsic    = _config_cache->numberOfRowsReadPerAsic();
    unsigned fixed_gain_idx = _config_cache->numberOfFixedGainModes() - 1;
    unsigned aMask          = _config_cache->asicMask();
    ndarray<unsigned,2> data    = make_ndarray<unsigned>(rows, cols);
    ndarray<unsigned,2> gstatus = make_ndarray<unsigned>(rows, cols);
    ndarray<const uint16_t,2> pixelGainConfig = _config_cache->pixelGainConfig();
    if (aMask==0) return;

    for(unsigned j=0; j<rows; j++) {
      for(unsigned k=0; k<cols; k++) {
        // The gain index to use is the highest of the set bits
        bool is_switch_mode = false;
        unsigned gain_val = (a(j,k) & gain_bits) >> 14;
        unsigned gain_idx = pixelGainConfig(j,k);
        double calib_val = (double) ((a(j,k) & data_bits) + offset);

        // Check if the pixel is in a gain switching mode
        is_switch_mode = gain_idx > fixed_gain_idx;
        // If the pixel is in a gain switching mode update the index
        if (is_switch_mode && gain_val)
          gain_idx += 2;

        if (!(d.options()&GainSwitchCalib::option_no_pedestal()))
          calib_val -= _pedestals(gain_idx,j,k);

        if (calib_val < 0.0) { // mask the problem negative pixels to avoid int underflow
          gstatus(j,k) = 1;
          data(j,k) = 0;
        } else {
          // combined status mask with set of gain switched pixels to not use in common mode
          gstatus(j,k) = _status(j,k) | (is_switch_mode ? gain_val : 0);
          data(j,k) = unsigned(calib_val + 0.5);
        }
      }
    }

    for(unsigned i=0; i<cal.shape()[0]; i++) {
      EntryWaveform* entry = _ewf[i];
      const uint16_t* v = &cal(i,0);
      for(unsigned j=0; j<cal.shape()[1]; j++)
        entry->content(double(*v++),j);
      entry->info(1,EntryWaveform::Normalization);
      entry->valid(t);
    }
    if (_ref)
      _ref->valid(t);

    int index = 0;
#if 1
    for(unsigned ic=0; ic<_config_cache->numberOfAsics(); ic++)
      _cache.cache(_feature[index++],double(temps[ic]));
#else
    if (aMask&1)
      _cache.cache(_feature[index+0],_tps_temp(temps[0]));
    if (aMask&2)
      _cache.cache(_feature[index+1],_therm.getTemp(temps[1]));
    if (aMask&4)
      _cache.cache(_feature[index+2],_therm.getTemp(temps[2]));
    if (aMask&8)
      _cache.cache(_feature[index+3],_tps_temp(temps[3]));
    index += 4;
#endif

    if (d.options()&FrameCalib::option_correct_common_mode2()) {
      for(unsigned i=0; i<nchip_rows; i++) {
        for(unsigned j=0; j<nchip_columns; j++) {
          if ((aMask&(aMask-1))!=0 && (aMask&(1<<_asic_map[i*nchip_columns+j]))==0)
            continue;
          unsigned shape[2];
          shape[0] = rowsPerAsic;
          shape[1] = colsPerAsic/4;
          for(unsigned m=0; m<4; m++) {
            for(unsigned y=0; y<rowsPerAsic; y++) {
              ndarray<uint32_t,1> s(&data (y+i*shape[0],(m+4*j)*shape[1]),&shape[1]);
              ndarray<const uint32_t,1> t(&gstatus(y+i*shape[0],(m+4*j)*shape[1]),&shape[1]);
              int fn = int(FrameCalib::frameNoise(s,t,offset));
              if (Ami::EventHandler::post_diagnostics() && m==0 && y<80)
                _cache.cache(_feature[(y%16)+index],double(fn));
              for(unsigned x=0; x<shape[1]; x++) {
                if (t[x] == 0)
                  s[x] -= fn;
              }
            }
          }
        }
      }
    }

    if (d.options()&FrameCalib::option_correct_common_mode()) {
      for(unsigned i=0; i<nchip_rows; i++) {
        for(unsigned j=0; j<nchip_columns; j++) {
          if ((aMask&(aMask-1))!=0 && (aMask&(1<<_asic_map[i*nchip_columns+j]))==0)
            continue;
          unsigned shape[2];
          shape[0] = rowsPerAsic;
          shape[1] = colsPerAsic/4;
          for(unsigned m=0; m<4; m++) {
            ndarray<uint32_t,2> s(&data (i*shape[0],(m+4*j)*shape[1]),shape);
            s.strides(data.strides());
            ndarray<uint32_t,2> t(&gstatus(i*shape[0],(m+4*j)*shape[1]),shape);
            t.strides(data.strides());
            int fn = int(FrameCalib::frameNoise(s,t,offset));
            for(unsigned y=0; y<shape[0]; y++) {
              for(unsigned x=0; x<shape[1]; x++) {
                if (t(y,x) == 0)
                  s(y,x) -= fn;
              }
            }
            if (Ami::EventHandler::post_diagnostics())
              _cache.cache(_feature[4*(i*nchip_columns+j)+m+index],double(fn));
          }
        }
      }
    }

    if (d.options()&FrameCalib::option_correct_common_mode3()) {
      for(unsigned i=0; i<nchip_rows; i++) {
        for(unsigned j=0; j<nchip_columns; j++) {
          if ((aMask&(aMask-1))!=0 && (aMask&(1<<_asic_map[i*nchip_columns+j]))==0)
            continue;
          unsigned shape[2];
          shape[0] = rowsPerAsic;
          shape[1] = colsPerAsic;
          for(unsigned x=0; x<shape[1]; x++) {
            ndarray<uint32_t,1> s(&data (i*shape[0],x+j*shape[1]),shape);
            s.strides(data.strides());
            ndarray<uint32_t,1> t(&gstatus(i*shape[0],x+j*shape[1]),shape);
            t.strides(data.strides());
            int fn = int(FrameCalib::frameNoise(s,t,offset));
            //if (Ami::EventHandler::post_diagnostics() && m==0 && y<80)
            //  _cache.cache(_feature[(y%16)+index],double(fn));
            for(unsigned y=0; y<shape[0]; y++) {
              if (t[y] == 0)
                s[y] -= fn;
            }
          }
        }
      }
    }

    //
    //  Correct for gain
    //
    if (d.options()&GainSwitchCalib::option_correct_gain()) {
      for(unsigned j=0; j<rows; j++) {
        for(unsigned k=0; k<cols; k++) {
          unsigned gain_idx = pixelGainConfig(j,k);
          if ((gain_idx > fixed_gain_idx) && ((a(j,k) & gain_bits) >> 14))
            gain_idx += 2;
          double calib_val = (data(j,k) - doffset) / _gains(gain_idx,j,k) + doffset;
          if (calib_val < 0.0) { // mask the problem negative pixels to avoid int underflow
            data(j,k) = 0;
          } else {
            data(j,k) = unsigned(calib_val +0.5);
          }
        }
      }
    }

    //
    //  Special case of one ASIC
    //
    if ((aMask&(aMask-1))==0) {
      unsigned asic=0;
      while((aMask&(1<<asic))==0)
        asic++;
      unsigned i = _asicLocation[asic].row;
      for(unsigned j=0; j<rowsPerAsic; j++) {
        unsigned r = i*rowsPerAsic+j;
        unsigned m = _asicLocation[asic].col*colsPerAsic;
        const SubFrame& fr = _desc.frame(0);
        for(unsigned k=0; k<colsPerAsic; k++) {
          _entry->addcontent(data(r,m+k), fr.x+k/ppbin, fr.y+j/ppbin);
        }
      }
    } else {
      for(unsigned j=0; j<rows; j++) {
        for(unsigned k=0; k<cols; k++) {
          _entry->addcontent(data(j,k), k/ppbin, j/ppbin);
        }
      }
    }

    _entry->info(double(offset*d.ppxbin()*d.ppybin()),EntryImage::Pedestal);
    _entry->info(norm,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void EpixHandler::_damaged() {
  if (_entry) _entry->invalid();
  if (_ref  ) _ref  ->invalid();
}

void EpixHandler::_load_pedestals()
{
  bool failed = false;
  unsigned rows         = _config_cache->numberOfRowsRead();
  unsigned cols         = _config_cache->numberOfColumns();
  unsigned colsPerAsic  = _config_cache->numberOfColumnsPerAsic();
  unsigned rowsPerAsic  = _config_cache->numberOfRowsReadPerAsic();
  unsigned gains        = _config_cache->numberOfGainModes();
  unsigned aMask        = _config_cache->asicMask();
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  if (aMask==0) return;

  if ((aMask&(aMask-1))==0) {
    unsigned asic=0;
    while((aMask&(1<<asic))==0)
      asic++;
    bool failed = false;
    // Try to find files to fill the status array assuming there is one for each gain!
    _status = GainSwitchCalib::load_array_sum(det, gains, rows, cols, 0, &failed, "sta", "pixel_status");
    if (failed) {
      _status = GainSwitchCalib::load_array(det, rows, cols, 0, &failed, "sta", "pixel_status");
      // Try if there is a single asic version of the status file...
      if (failed) {
        ndarray<unsigned,2> asic_status = GainSwitchCalib::load_array(det, rowsPerAsic, colsPerAsic, 0, &failed, "sta", "pixel_status");
        if (!failed) {
          _load_one_asic(asic_status, asic, _status);
        }
      }
    }
    _pedestals = GainSwitchCalib::load_multi_array(det, gains, rows, cols, 0.0, &failed, "ped", "pedestals", true);
    if (failed) {
      ndarray<double,3> asic_pedestals = GainSwitchCalib::load_multi_array(det, gains, rowsPerAsic, colsPerAsic, 0.0, &failed, "ped", "pedestals", true);
      if (!failed) {
        _load_one_asic(asic_pedestals, asic, _pedestals);
      }
    }
  } else {
    // Try to find files to fill the status array assuming there is one for each gain!
    _status = GainSwitchCalib::load_array_sum(det, gains, rows, cols, 0, &failed, "sta", "pixel_status");
    if (failed) {
      _status = GainSwitchCalib::load_array(det, rows, cols, 0, NULL, "sta", "pixel_status");
    }
    _pedestals = GainSwitchCalib::load_multi_array(det, gains, rows, cols, 0.0, NULL, "ped", "pedestals", true);
  }
}

void EpixHandler::_load_gains()
{
  bool failed = false;
  unsigned rows         = _config_cache->numberOfRowsRead();
  unsigned cols         = _config_cache->numberOfColumns();
  unsigned colsPerAsic  = _config_cache->numberOfColumnsPerAsic();
  unsigned rowsPerAsic  = _config_cache->numberOfRowsReadPerAsic();
  unsigned gains        = _config_cache->numberOfGainModes();
  unsigned aMask        = _config_cache->asicMask();
  const unsigned gain_cor[] = { 1, 3, 100, 1, 3, 100, 100 };
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  if (aMask==0) return;

  if ((aMask&(aMask-1))==0) {
    unsigned asic=0;
    while((aMask&(1<<asic))==0)
      asic++;
    _gains = GainSwitchCalib::load_multi_array(det, gains, rows, cols, 1.0, &failed, "gain", "pixel_gain");
    if (failed) {
      ndarray<double,3> asic_gains = GainSwitchCalib::load_multi_array(det, gains, rowsPerAsic, colsPerAsic, 1.0, &failed, "gain", "pixel_gain");
      if (!failed) {
        _load_one_asic(asic_gains, asic, _gains);
      }
    }
  } else {
    _gains = GainSwitchCalib::load_multi_array(det, gains, rows, cols, 1.0, &failed, "gain", "pixel_gain");
  }
  if (failed) {
    printf("No valid pixel gain correction file found: using the default corrections!\n");
    _do_norm = false;
    for(unsigned g=0; g<gains; g++)
      for(unsigned j=0; j<rows; j++)
        for(unsigned k=0; k<cols; k++)
          _gains(g,j,k) = (1.0 / gain_cor[g]);
  } else {
    _do_norm = true;
    for(double* val = _gains.begin(); val!=_gains.end(); val++)
      (*val) /= prec_norm_factor;
  }
}

void _load_one_asic(ndarray<unsigned,2>& asic_array,
                    unsigned asic,
                    ndarray<unsigned,2>& full_array)
{
  unsigned rowsPerAsic = asic_array.shape()[0];
  unsigned colsPerAsic = asic_array.shape()[1];
  unsigned i = _asicLocation[asic].row;
  for(unsigned j=0; j<rowsPerAsic; j++) {
    unsigned r = i*rowsPerAsic+j;
    unsigned m = _asicLocation[asic].col*colsPerAsic;
    for(unsigned k=0; k<colsPerAsic; k++) {
      full_array(r,m+k) = asic_array(j,k);
    }
  }
}

void _load_one_asic(ndarray<double,3>& asic_array,
                    unsigned asic,
                    ndarray<double,3>& full_array)
{
  unsigned gains = asic_array.shape()[0];
  unsigned rowsPerAsic = asic_array.shape()[1];
  unsigned colsPerAsic = asic_array.shape()[2];
  for (unsigned g=0; g<gains; g++) {
    unsigned i = _asicLocation[asic].row;
    for(unsigned j=0; j<rowsPerAsic; j++) {
      unsigned r = i*rowsPerAsic+j;
      unsigned m = _asicLocation[asic].col*colsPerAsic;
      for(unsigned k=0; k<colsPerAsic; k++) {
        full_array(g,r,m+k) = asic_array(g,j,k);
      }
    }
  }
}

double _tps_temp(const uint16_t q)
{
  const double degCperADU = 1./28.1;
  const double t0 = -489.28;
  return double(q)*degCperADU + t0;
}

EpixAmi::EnvData1::EnvData1(EventHandlerF& h) :
  _handler(h), _index(make_ndarray<int>(5)) {}

void     EpixAmi::EnvData1::addFeatures(const char* name)
{
#define ADDV(s) {                                               \
    std::ostringstream ostr;                                    \
    ostr << name << ":Epix:" << #s;                             \
      _index[index++] = _handler._add_to_cache(ostr.str().c_str()); }

  unsigned index=0;
  ADDV(AVDD);
  ADDV(DVDD);
  ADDV(AnaCardT);
  ADDV(StrBackT);
  ADDV(Humidity);
#undef ADDV
}

void     EpixAmi::EnvData1::rename     (const char* name)
{
#define ADDV(s) {                                               \
    std::ostringstream ostr;                                    \
    ostr << name << ":Epix:" << #s;                             \
      _handler._rename_cache(_index[index++],ostr.str().c_str()); }

  unsigned index=0;
  ADDV(AVDD);
  ADDV(DVDD);
  ADDV(AnaCardT);
  ADDV(StrBackT);
  ADDV(Humidity);
#undef ADDV
}

void      EpixAmi::EnvData1::fill      (FeatureCache& cache,
                                        ndarray<const uint16_t,2> env)
{
  const uint16_t* last = &env(env.shape()[0]-1,0);
  unsigned index=0;
  cache.cache(_index[index++], double(last[2])*0.00183);
  cache.cache(_index[index++], double(last[3])*0.00183);
  cache.cache(_index[index++], double(last[4])*(-0.0194) + 78.393);
  cache.cache(_index[index++], _therm.getTemp(last[6]));
  cache.cache(_index[index++], double(last[7])*0.0291 - 23.8);
}

EpixAmi::EnvData2::EnvData2(EventHandlerF& h) :
  _handler(h), _index(make_ndarray<int>(9)) {}

void     EpixAmi::EnvData2::addFeatures(const char* name)
{
#define ADDV(s) {                                               \
    std::ostringstream ostr;                                    \
    ostr << name << ":Epix:" << #s;                             \
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

void     EpixAmi::EnvData2::rename     (const char* name)
{
#define ADDV(s) {                                               \
    std::ostringstream ostr;                                    \
    ostr << name << ":Epix:" << #s;                             \
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

void      EpixAmi::EnvData2::fill      (FeatureCache& cache,
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
