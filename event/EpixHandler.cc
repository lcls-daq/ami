#include "EpixHandler.hh"

#include "ami/event/FrameCalib.hh"
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
static const double  RDIV=30000;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_EpixConfig);
  types.push_back(Pds::TypeId::Id_Epix10kConfig);
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

static void _load_one_asic(ndarray<unsigned,2>& pb, 
			   unsigned ny,
			   ndarray<unsigned,2>& pedestals);
static void _load_one_asic(ndarray<double,2>& pb, 
			   unsigned ny,
			   ndarray<double,2>& pedestals);
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
    unsigned numberOfAsics() const  { return _nAsics; }
    unsigned asicMask      () const { return _aMask; }
    EnvData* envData       () const { return _envData; }
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
  public:
    void event(const void* payload, 
	       ndarray<const uint16_t,2>& frame, 
	       ndarray<const uint16_t,1>& temps,
	       ndarray<const uint16_t,2>& cal,
               FeatureCache&              cache) {
#define PARSE_CONFIG(typ,dtyp,etyp)  					\
      const dtyp& f = *reinterpret_cast<const dtyp*>(payload);		\
      const typ& c = *reinterpret_cast<const typ*>(_buffer);		\
      frame = f.frame(c);                                               \
      temps = f.temperatures(c);                                        \
      _envData->fill(cache,f.etyp(c));

      switch(_id.id()) {
      case Pds::TypeId::Id_EpixConfig   : 
	{ PARSE_CONFIG(Pds::Epix::ConfigV1    ,Pds::Epix::ElementV1, excludedRows) }
        break;
      case Pds::TypeId::Id_Epix10kConfig: 
	{ PARSE_CONFIG(Pds::Epix::Config10KV1 ,Pds::Epix::ElementV1, excludedRows); }
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
	default:
	break;
      }
#undef PARSE_CONFIG
    }
    void dump() const;
  private:
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
    EnvData* _envData;
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

  if (tid.id()==Pds::TypeId::Id_GenericPgpConfig) {
    const Pds::GenericPgp::ConfigV1& c = *reinterpret_cast<const Pds::GenericPgp::ConfigV1*>(payload);
    reinterpret_cast<uint32_t&>(tid) = c.stream()[0].config_type();
    payload = reinterpret_cast<const void*>(c.payload().data()+c.stream()[0].config_offset());
  }

#define PARSE_CONFIG(typ) 						\
  const typ& c = *reinterpret_cast<const typ*>(payload);		\
  _buffer = new char[c._sizeof()];					\
  memcpy(_buffer, &c, c._sizeof());					\
  _nchip_columns=c.numberOfAsicsPerRow();				\
  _nchip_rows   =c.numberOfAsicsPerColumn();				\
  _columns = c.numberOfColumns();					\
  _colsPerAsic = c.numberOfPixelsPerAsicRow();				\
  _rows = c.numberOfRows();						\
  _rowsPerAsic = c.numberOfRowsPerAsic();				\
  _aMask  = c.asicMask() ? c.asicMask() : 0xf;                          \
  _nAsics = c.numberOfAsics();                                          \
  _envData = c.version() < 0xea020000 ? (EnvData*)new EnvData1(*handler) : (EnvData*)new EnvData2(*handler);

  switch(_id.id()) {
  case Pds::TypeId::Id_EpixConfig   : 
    { PARSE_CONFIG(Pds::Epix::ConfigV1);
      _rowsRead        = _rows;
      _rowsReadPerAsic = _rows/_nchip_rows; } break;
  case Pds::TypeId::Id_Epix10kConfig:
    { PARSE_CONFIG(Pds::Epix::Config10KV1);
      _rowsRead        = _rows;
      _rowsReadPerAsic = _rows/_nchip_rows; } break;
  case Pds::TypeId::Id_Epix100aConfig: 
    switch(_id.version()) {
    case 1:
      { PARSE_CONFIG(Pds::Epix::Config100aV1);
	_rowsRead        = c.numberOfReadableRows();
	_rowsReadPerAsic = c.numberOfReadableRowsPerAsic();
	_rowsCal         = c.numberOfCalibrationRows();
	_rowsCalPerAsic  = _rowsCal/c.numberOfAsicsPerColumn(); } break;
    case 2:
      { PARSE_CONFIG(Pds::Epix::Config100aV2);
	_rowsRead        = c.numberOfReadableRows();
	_rowsReadPerAsic = c.numberOfReadableRowsPerAsic();
	_rowsCal         = c.numberOfCalibrationRows();
	_rowsCalPerAsic  = _rowsCal/c.numberOfAsicsPerColumn(); } break;
    default:
      printf("Epix100aConfig version %d unknown\n",_id.version());
      break;
    }
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
  _rowsCalPerAsic (o._rowsCalPerAsic)
{
  unsigned sz=0;
#define PARSE_CONFIG(typ) 						\
  const typ& c = *reinterpret_cast<const typ*>(o._buffer);		\
  sz = c._sizeof();

  switch(_id.id()) {
  case Pds::TypeId::Id_EpixConfig   : 
    { PARSE_CONFIG(Pds::Epix::ConfigV1) } break;
  case Pds::TypeId::Id_Epix10kConfig: 
    { PARSE_CONFIG(Pds::Epix::Config10KV1) } break;
  case Pds::TypeId::Id_Epix100aConfig: 
    { PARSE_CONFIG(Pds::Epix::Config100aV1) } break;
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

static double frameNoise(ndarray<const uint32_t,2> data,
                         ndarray<const uint32_t,2> status,
			 unsigned off)
{
  const int fnPixelMin = -100 + off;
  const int fnPixelMax =  100 + off;
  const int fnPixelBins = fnPixelMax - fnPixelMin;
  const int peakSpace   = 5;
  
  //  histogram the pixel values
  unsigned hist[fnPixelBins];
  unsigned nhist=0;
  { memset(hist, 0, fnPixelBins*sizeof(unsigned));
    for(unsigned i=0; i<data.shape()[0]; i++) {
      for(unsigned j=0; j<data.shape()[1]; j++) {
        if (status(i,j)==0) {
          nhist++;
          int v = data(i,j) - fnPixelMin;
          if (v >= 0 && v < int(fnPixelBins))
            hist[v]++;
        }
      }
    }
  }

  double v = 0;
  // the first peak from the left above this is the pedestal
  { const int fnPeakBins = 5;
    const int fnPixelRange = fnPixelBins-fnPeakBins-1;
    const unsigned fnPedestalThreshold = nhist>>5;
    
    unsigned i=fnPeakBins;
    while( int(i)<fnPixelRange ) {
      if (hist[i]>fnPedestalThreshold) break;
      i++;
    }

    unsigned thresholdPeakBin=i;
    unsigned thresholdPeakBinContent=hist[i];
    while( int(++i)<fnPixelRange ) {
      if (hist[i]<thresholdPeakBinContent) {
        if (i > thresholdPeakBin+peakSpace)
          break;
      }
      else {
        thresholdPeakBin = i;
        thresholdPeakBinContent = hist[i];
      }
    }

    i = thresholdPeakBin;
    if ( int(i)+fnPeakBins<=fnPixelRange ) {
      unsigned s0 = 0;
      unsigned s1 = 0;
      for(unsigned j=i-fnPeakBins+1; j<i+fnPeakBins; j++) {
        s0 += hist[j];
        s1 += hist[j]*j;
      }
      
      double binMean = double(s1)/double(s0);
      v =  binMean + fnPixelMin - off;
      
      s0 = 0;
      unsigned s2 = 0;
      for(unsigned j=i-10; j<i+fnPeakBins; j++) {
        s0 += hist[j];
	s2 += hist[j]*(j-int(binMean))*(j-int(binMean));
      }
//      const double allowedPedestalWidthSquared = 2.5*2.5;
      //      printf("frameNoise finds mean %f, variance %f\n", v, double(s2)/double(s0));
//      if (double(s2)/double(s0)>allowedPedestalWidthSquared) v = 0;
      // this isn't the standard rms around the mean, but should be similar if rms_real < 3
      //      printf("frameNoise finds mean %f, variance %f\n", v, double(s2)/double(s0));

    }
    else {
      static unsigned nPrint=0;
      nPrint++;
      if ((nPrint<10) || (nPrint%100)==0)
        printf("frameNoise : peak not found [%d]\n",nPrint);
    }
//    printf("CspadMiniHandler::frameNoise v=%lf\n", v);
  }

  return v;
}

static int frameNoise(ndarray<const uint32_t,1> data,
                      ndarray<const uint32_t,1> status,
		      unsigned off)
{
  unsigned lo = off-100;
  unsigned hi = off+100;
  return FrameCalib::median(data,status,lo,hi)-int(off);
}


EpixHandler::EpixHandler(const Pds::Src& info, FeatureCache& cache) :
  EventHandlerF  (info, Data_Type.id(), config_type_list(), cache),
  _cache        (cache),
  _desc         ("template",0,0),
  _entry        (0),
  _pentry       (0),
  _ref          (0),
  _options      (0),
  _pedestals    (make_ndarray<unsigned>(1,1)),
  _pedestals_lo (make_ndarray<unsigned>(1,1)),
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

  if (_pentry) { 
    delete _pentry; 
    _pentry=0; 
  }
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

  { int ppb = 1;
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, _config_cache->numberOfRows(), ppb, ppb);

    _pentry = new EntryImage(desc);
    _pentry->invalid();
  }

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

    if (_entry->desc().options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( _entry->desc().options()&~FrameCalib::option_reload_pedestal() );
    }

    ndarray<const uint16_t,2> a;
    ndarray<const uint16_t,1> temps;
    ndarray<const uint16_t,2> cal;

    _config_cache->event(payload, a, temps, cal, _cache);

    _entry->reset();
    const DescImage& d = _entry->desc();

    unsigned nskip = (_config_cache->numberOfRows()-a.shape()[0])/_config_cache->numberOfAsicsPerColumn();

    ndarray<const unsigned,2> pa =
      d.options()&FrameCalib::option_no_pedestal() ?
      make_ndarray(&_offset   (nskip,0),a.shape()[0],a.shape()[1]) : 
      make_ndarray(&_pedestals(nskip,0),a.shape()[0],a.shape()[1]);

    ndarray<const unsigned,2> pa_lo =
      d.options()&FrameCalib::option_no_pedestal() ?
      make_ndarray(&_offset      (nskip,0),a.shape()[0],a.shape()[1]) : 
      make_ndarray(&_pedestals_lo(nskip,0),a.shape()[0],a.shape()[1]);

    ndarray<const unsigned,2> sta =
      make_ndarray(&_status      (nskip,0),a.shape()[0],a.shape()[1]);

    int ppbin = _entry->desc().ppxbin();

    //
    //  Special case of one ASIC
    //
    unsigned aMask = _config_cache->asicMask();
    if (aMask==0) return;

    if ((aMask&(aMask-1))==0) {
      unsigned asic=0;
      while((aMask&(1<<asic))==0)
	asic++;
      unsigned i = _asicLocation[asic].row;
      for(unsigned j=0; j<_config_cache->numberOfRowsReadPerAsic(); j++) {
	unsigned r = i*_config_cache->numberOfRowsReadPerAsic()+j;
	unsigned m = _asicLocation[asic].col*_config_cache->numberOfColumnsPerAsic();
	const uint16_t* d = & a(r,m);
	const unsigned* p_hi = &pa   (j,0);
	const unsigned* p_lo = &pa_lo(j,0);
        const unsigned* s    = &sta  (r,m);
	const SubFrame& fr = _desc.frame(0);
	for(unsigned k=0; k<_config_cache->numberOfColumnsPerAsic(); k++) {
          unsigned v = s[k]==0 ? ((d[k]&0x4000) ? 
                                  unsigned(d[k]&0x3fff) + p_lo[k] :
                                  unsigned(d[k]&0x3fff) + p_hi[k]) :
            offset;
          _entry->addcontent(v, fr.x+k/ppbin, fr.y+j/ppbin);
	}
      }
    }
    else {
      for(unsigned i=0; i<_config_cache->numberOfAsicsPerColumn(); i++)
	for(unsigned j=0; j<_config_cache->numberOfRowsReadPerAsic(); j++) {
	  unsigned r = i*_config_cache->numberOfRowsReadPerAsic()+j;
	  for(unsigned m=0; m<_config_cache->numberOfAsicsPerRow(); m++) {
	    unsigned fn = i*_config_cache->numberOfAsicsPerRow()+m;
	    if (aMask & 1<<_asic_map[fn]) {
	      unsigned q = m*_config_cache->numberOfColumnsPerAsic();
	      const uint16_t* d    = &a    (r,q);
	      const unsigned* p_hi = &pa   (r,q);
	      const unsigned* p_lo = &pa_lo(r,q);
              const unsigned* s    = &sta  (r,q);
	      const SubFrame& fr = _desc.frame(fn);
	      for(unsigned k=0; k<_config_cache->numberOfColumnsPerAsic(); k++) {
                unsigned v = s[k]==0 ? ((d[k]&0x4000) ? 
                                        unsigned(d[k]&0x3fff) + p_lo[k] :
                                        unsigned(d[k]&0x3fff) + p_hi[k]) :
                  offset;
                _entry->addcontent(v, fr.x+k/ppbin, fr.y+j/ppbin);
	      }
	    }
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
      for(unsigned k=0; k<_desc.nframes(); k++) {
        if ((aMask&(aMask-1))!=0 && (aMask&(1<<_asic_map[k]))==0) 
          continue;
        ndarray<uint32_t,2> e = _entry->contents(_desc.frame(k));
        unsigned shape[2];
        shape[0] = e.shape()[0];
        shape[1] = e.shape()[1]/4;
        for(unsigned m=0; m<4; m++) {
          for(unsigned y=0; y<shape[0]; y++) {
            ndarray<const uint32_t,1> s(&e  (y,m*shape[1]),&shape[1]);
            ndarray<const uint32_t,1> t(&sta(y,m*shape[1]),&shape[1]);
            int fn = int(frameNoise(s,t,offset*int(d.ppxbin()*d.ppybin()+0.5)));
            if (Ami::EventHandler::post_diagnostics() && m==0 && y<80)
              _cache.cache(_feature[(y%16)+index],double(fn));
            uint32_t* v = const_cast<uint32_t*>(&s[0]);
            for(unsigned x=0; x<shape[1]; x++)
              v[x] -= fn;
          }
        }
      }	
    }

    if (d.options()&FrameCalib::option_correct_common_mode()) {
      for(unsigned k=0; k<_desc.nframes(); k++) {
        if ((aMask&(aMask-1))!=0 && (aMask&(1<<_asic_map[k]))==0) 
          continue;
        ndarray<uint32_t,2> e = _entry->contents(_desc.frame(k));
	unsigned shape[2];
	shape[0] = e.shape()[0];
	shape[1] = e.shape()[1]/4;
	for(unsigned m=0; m<4; m++) {
	  ndarray<uint32_t,2> s(&e(0,m*shape[1]),shape);
	  s.strides(e.strides());
          ndarray<uint32_t,2> t(_status.begin()+(s.begin()-_entry->content().begin()),shape);
          t.strides(_status.strides());
	  int fn = int(frameNoise(s,t,offset*int(d.ppxbin()*d.ppybin()+0.5)));
	  for(unsigned y=0; y<shape[0]; y++) {
	    uint32_t* v = &s(y,0);
	    for(unsigned x=0; x<shape[1]; x++)
	      v[x] -= fn;
	  }
	  if (Ami::EventHandler::post_diagnostics())
	    _cache.cache(_feature[4*k+m+index],double(fn));
	}
      }	
    }

    if (d.options()&FrameCalib::option_correct_common_mode3()) {
      for(unsigned k=0; k<_desc.nframes(); k++) {
        if ((aMask&(aMask-1))!=0 && (aMask&(1<<_asic_map[k]))==0) 
          continue;
        ndarray<uint32_t,2> e = _entry->contents(_desc.frame(k));
	for(unsigned x=0; x<e.shape()[1]; x++) {
	  ndarray<uint32_t,1> s(&e  (0,x),e.shape());
	  s.strides(e.strides());
          ndarray<const uint32_t,1> t(&sta(0,x),e.shape());
	  t.strides(e.strides());
	  int fn = int(frameNoise(s,t,offset*int(d.ppxbin()*d.ppybin()+0.5)));
	  //if (Ami::EventHandler::post_diagnostics() && m==0 && y<80)
	  //  _cache.cache(_feature[(y%16)+index],double(fn));
	  for(unsigned y=0; y<e.shape()[0]; y++)
	    s[y] -= fn;
        }
      }	
    }

    //
    //  Correct for gain
    //
    ndarray<const double,2> gn    = 
      (d.options()&FrameCalib::option_correct_gain()) ? 
      make_ndarray(&_gain   (nskip,0),a.shape()[0],a.shape()[1]) : 
      make_ndarray(&_no_gain(nskip,0),a.shape()[0],a.shape()[1]);

    ndarray<const double,2> gn_lo = 
      (d.options()&FrameCalib::option_correct_gain()) ?
      make_ndarray(&_gain_lo(nskip,0),a.shape()[0],a.shape()[1]) : 
      make_ndarray(&_no_gain(nskip,0),a.shape()[0],a.shape()[1]);
    {
      for(unsigned k=0; k<_desc.nframes(); k++) {
        if ((aMask&(aMask-1))!=0 && (aMask&(1<<_asic_map[k]))==0) 
	  continue;
	ndarray<uint32_t,2> e = _entry->contents(_desc.frame(k));
	unsigned r = (k/_config_cache->numberOfAsicsPerRow()) * _config_cache->numberOfRowsReadPerAsic();
	unsigned m = (k%_config_cache->numberOfAsicsPerRow()) * _config_cache->numberOfColumnsPerAsic();
	for(unsigned y=0; y<e.shape()[0]; y++,r++) {
	  uint32_t*        v = &e    (y,0);
	  const uint16_t*  d = &a    (r,m);
	  const double* g_hi = &gn   (r,m);
	  const double* g_lo = &gn_lo(r,m);
	  for(unsigned x=0; x<e.shape()[1]; x++) {
	    double q = (double(v[x]) - doffset) * ( (d[x]&0x4000) ? g_lo[x]:g_hi[x] );
	    v[x] = unsigned(q+offset);
	  }
	}
      }	
    }

    _entry->info(double(offset*d.ppxbin()*d.ppybin()),EntryImage::Pedestal);
    _entry->info(1.,EntryImage::Normalization);
    _entry->valid(t);
  }
}
  
void EpixHandler::_damaged() { 
  if (_entry) _entry->invalid(); 
  if (_ref  ) _ref  ->invalid(); 
}

void EpixHandler::_load_pedestals()
{
  unsigned rows =_config_cache->numberOfRows();
  unsigned cols =_config_cache->numberOfColumns();
  unsigned aMask=_config_cache->asicMask();
  if (aMask==0) return;

  _status       = make_ndarray<unsigned>(rows,cols);
  _pedestals    = make_ndarray<unsigned>(rows,cols);
  _pedestals_lo = make_ndarray<unsigned>(rows,cols);
  _offset       = make_ndarray<unsigned>(rows,cols);
  for(unsigned* a = _offset.begin(); a!=_offset.end(); *a++ = offset) ;

  bool loffl=false;
  FILE* f = Ami::Calib::fopen(static_cast<const Pds::DetInfo&>(info()), 
                              "sta", "pixel_status", true, &loffl);
  if (f) {
    ndarray<unsigned,2>  pb = FrameCalib::load_array(f);
    if (loffl && pb.shape()[0]) {
      if ((aMask&(aMask-1))==0) {
        switch(aMask) {
        case 1: {
          for(unsigned i=0; i<rows/2; i++)
            for(unsigned j=0; j<cols/2; j++)
              _status(i,j) = pb(i+rows/2,j+cols/2);
        } break;
        case 2: {
          for(unsigned i=0; i<rows/2; i++)
            for(unsigned j=0; j<cols/2; j++)
              _status(i,j) = pb(i,j+cols/2);
        } break;
        case 4: {
          for(unsigned i=0; i<rows/2; i++)
            for(unsigned j=0; j<cols/2; j++)
              _status(i,j) = pb(i,j);
        } break;
        case 8: {
          for(unsigned i=0; i<rows/2; i++)
            for(unsigned j=0; j<cols/2; j++)
              _status(i,j) = pb(i+rows/2,j);
        } break;
        default: break;
        }
      }
      else {
        for(unsigned* a=_status.begin(), *b=pb.begin(); a!=_status.end(); a++,b++)
          *a = *b;
      }
    }
    fclose(f);
  }
  else {
    EntryImage* p = _pentry;
    ndarray<unsigned,2> pb = FrameCalib::load_array(p->desc(),"sta");
    if (pb.shape()[0] && pb.shape()[1]==p->desc().nbinsx() && pb.shape()[0]<=p->desc().nbinsy()) {
      unsigned nskip = (_status.shape()[0]-pb.shape()[0])/2;
      for(unsigned *a=pb.begin(), *b=&_status(nskip,0); a!=pb.end(); *b++=*a++) ;
      DescImage& d = _entry->desc();
      _load_one_asic(pb, d.nbinsy(), _status);
      ImageMask mask(d.nbinsy(),d.nbinsx());
      mask.fill();
      for(unsigned i=0; i<d.nbinsy(); i++)
        for(unsigned j=0; j<d.nbinsx(); j++)
          if (_status(nskip+i,j)) mask.clear(i,j);
      mask.update();
      d.set_mask(mask);

    }
    else if ((aMask&(aMask-1))==0 && pb.shape()[0] && pb.shape()[1]==_entry->desc().nbinsx()) {
      DescImage& d = _entry->desc();
      _load_one_asic(pb, d.nbinsy(), _status);
      ImageMask mask(d.nbinsy(),d.nbinsx());
      mask.fill();
      for(unsigned i=0; i<d.nbinsy(); i++)
        for(unsigned j=0; j<d.nbinsx(); j++)
          if (_status(i,j)) mask.clear(i,j);
      mask.update();
      d.set_mask(mask);
    }
    else
      for(unsigned* a=_status.begin(); a!=_status.end(); *a++=0) ;
  }

  f = Ami::Calib::fopen(static_cast<const Pds::DetInfo&>(info()), 
                        "ped", "pedestals", true, &loffl);
  printf("f %p  loffl %c\n",f, loffl ? 't':'f');
  if (f) {
    ndarray<double,2>  pb = FrameCalib::load_darray(f);
    if (/*loffl && */pb.shape()[0]) {
      printf("aMask %x  rows %u cols %u  shape %u %u\n",
             aMask, rows, cols, pb.shape()[0], pb.shape()[1]);
      if ((aMask&(aMask-1))==0 && pb.shape()[0]) {
        switch(aMask) {
        case 1: {
          for(unsigned i=0; i<rows/2; i++)
            for(unsigned j=0; j<cols/2; j++)
              _pedestals(i,j) = offset-unsigned(pb(i+rows/2,j+cols/2)+0.5);
        } break;
        case 2: {
          for(unsigned i=0; i<rows/2; i++)
            for(unsigned j=0; j<cols/2; j++)
              _pedestals(i,j) = offset-unsigned(pb(i,j+cols/2)+0.5);
        } break;
        case 4: {
          for(unsigned i=0; i<rows/2; i++)
            for(unsigned j=0; j<cols/2; j++)
              _pedestals(i,j) = offset-unsigned(pb(i,j)+0.5);
        } break;
        case 8: {
          for(unsigned i=0; i<rows/2; i++)
            for(unsigned j=0; j<cols/2; j++)
              _pedestals(i,j) = offset-unsigned(pb(i+rows/2,j)+0.5);
        } break;
        default: break;
        }
      }
      else {
        double* b = pb.begin();
        for(unsigned* a=_pedestals.begin(); a!=_pedestals.end(); a++,b++)
          *a = offset-unsigned(*b+0.5);
      }
    }
    else {
      for(unsigned* a=_pedestals.begin(); a!=_pedestals.end(); a++)
        *a = offset;
    }
    fclose(f);
  }
  else {
    EntryImage* p = _pentry;
    ndarray<unsigned,2> pb = FrameCalib::load_array(p->desc(),"ped");
    printf("pb size %d,%d\n",pb.shape()[0],pb.shape()[1]);
    if (pb.shape()[0] && pb.shape()[1]==p->desc().nbinsx() && pb.shape()[0]<=p->desc().nbinsy()) {
      unsigned nskip = (_pedestals.shape()[0]-pb.shape()[0])/2;
      for(unsigned *a=pb.begin(), *b=&_pedestals(nskip,0); a!=pb.end(); *b++=offset-*a++) ;
    }
    else if ((aMask&(aMask-1))==0 && pb.shape()[0] && pb.shape()[1]==_entry->desc().nbinsx()) {
      _load_one_asic(pb, _entry->desc().nbinsy(), _pedestals);
      for(unsigned* p=_pedestals.begin(); p!=_pedestals.end(); p++)
        *p = offset-*p;
    }
    else
      for(unsigned* a=_pedestals.begin(); a!=_pedestals.end(); *a++=offset) ;

    pb = FrameCalib::load_array(p->desc(),"ped_lo");
    if (pb.shape()[0] && pb.shape()[1]==p->desc().nbinsx() && pb.shape()[0]<=p->desc().nbinsy()) {
      unsigned nskip = (_pedestals_lo.shape()[0]-pb.shape()[0])/2;
      for(unsigned *a=pb.begin(), *b=&_pedestals_lo(nskip,0); a!=pb.end(); *b++=offset-*a++) ;
    }
    else if ((aMask&(aMask-1))==0 && pb.shape()[0] && pb.shape()[1]==_entry->desc().nbinsx()) {
      _load_one_asic(pb, _entry->desc().nbinsy(), _pedestals_lo);
      for(unsigned* p=_pedestals_lo.begin(); p!=_pedestals_lo.end(); p++)
        *p = offset-*p;
    }
    else
      for(unsigned* a=_pedestals_lo.begin(), *b=_pedestals.begin(); a!=_pedestals_lo.end(); *a++=*b++) ;
  }
}

void EpixHandler::_load_gains()
{
  unsigned rows =_config_cache->numberOfRows();
  unsigned cols =_config_cache->numberOfColumns();
  unsigned aMask=_config_cache->asicMask();
  if (aMask==0) return;

  _gain    = make_ndarray<double>(rows,cols);
  _gain_lo = make_ndarray<double>(rows,cols);
  _no_gain = make_ndarray<double>(rows,cols);

  for(double* a=_no_gain.begin(); a!=_no_gain.end(); *a++=1.) ;

  EntryImage* p = _pentry;
  ndarray<double,2> pb = FrameCalib::load_darray(p->desc(),"gain");
  if (pb.shape()[0] && pb.shape()[1]==p->desc().nbinsx() && pb.shape()[0]<=p->desc().nbinsy()) {
    unsigned nskip = (_gain.shape()[0]-pb.shape()[0])/2;
    for(double *a=pb.begin(), *b=&_gain(nskip,0); a!=pb.end(); *a++=*b++) ;
  }
  else if ((aMask&(aMask-1))==0 && pb.shape()[0] && pb.shape()[1]==_entry->desc().nbinsx()) {
    _load_one_asic(pb, _entry->desc().nbinsy(), _gain);
  }
  else
    for(double* a=_gain.begin(); a!=_gain.end(); *a++=1.) ;

  pb = FrameCalib::load_darray(p->desc(),"gain_lo");
  if (pb.shape()[0] && pb.shape()[1]==p->desc().nbinsx() && pb.shape()[0]<=p->desc().nbinsy()) {
    unsigned nskip = (_gain_lo.shape()[0]-pb.shape()[0])/2;
    for(double *a=pb.begin(), *b=&_gain_lo(nskip,0); a!=pb.end(); *a++=*b++) ;
  }
  else if ((aMask&(aMask-1))==0 && pb.shape()[0] && pb.shape()[1]==_entry->desc().nbinsx()) {
    _load_one_asic(pb, _entry->desc().nbinsy(), _gain_lo);
  }
  else
    for(double* a=_gain_lo.begin(); a!=_gain_lo.end(); *a++=100.) ;
}

void _load_one_asic(ndarray<unsigned,2>& pb, 
		    unsigned ny,
		    ndarray<unsigned,2>& pedestals)
{
  printf("_load_one_asic [%ux%u] [%ux%u] ny %u\n",
	 pb.shape()[0],pb.shape()[1],
	 pedestals.shape()[0],pedestals.shape()[1],
	 ny);

  unsigned nskip = (ny-pb.shape()[0])/2;
  unsigned nx = pb.shape()[1];

  for(unsigned i=0; i<pb.shape()[0]; i++) {
    unsigned* a = &pb(i,0);
    unsigned* b0 = &pedestals(nskip+i   ,0);
    unsigned* b1 = &pedestals(nskip+i+ny,0);
    for(unsigned j=0; j<pb.shape()[1]; j++) {
	unsigned v = a[j];
	b0[j   ] = v;
	b0[j+nx] = v;
	b1[j   ] = v;
	b1[j+nx] = v;
    }
  }
}

void _load_one_asic(ndarray<double,2>& pb, 
		    unsigned ny,
		    ndarray<double,2>& pedestals)
{
  printf("_load_one_asic [%ux%u] [%ux%u] ny %u\n",
	 pb.shape()[0],pb.shape()[1],
	 pedestals.shape()[0],pedestals.shape()[1],
	 ny);

  unsigned nskip = (ny-pb.shape()[0])/2;
  unsigned nx = pb.shape()[1];
  for(unsigned i=0; i<pb.shape()[0]; i++) {
    double* a = &pb(i,0);
    double* b0 = &pedestals(nskip+i   ,0);
    double* b1 = &pedestals(nskip+i+ny,0);
    for(unsigned j=0; j<pb.shape()[1]; j++) {
	double v = a[j];
	b0[j   ] = v;
	b0[j+nx] = v;
	b1[j   ] = v;
	b1[j+nx] = v;
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
