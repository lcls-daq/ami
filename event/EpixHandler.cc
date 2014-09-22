#include "EpixHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/ImageMask.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/genericpgp.ddl.h"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/xtc/ClockTime.hh"

#include <string.h>

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

static double _tps_temp(const uint16_t q);

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
};

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
        if (status[i][j]==0) {
          nhist++;
          int v = data[i][j] - fnPixelMin;
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
      for(unsigned j=i-fnPeakBins-1; j<i+fnPeakBins; j++) {
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
		      unsigned off)
{
  unsigned lo = off-100;
  unsigned hi = off+100;
  return FrameCalib::median(data,lo,hi)-int(off);
}


EpixHandler::EpixHandler(const Pds::Src& info, FeatureCache& cache) :
  EventHandlerF  (info, Data_Type.id(), config_type_list(), cache),
  _cache        (cache),
  _desc         ("template",0,0),
  _entry        (0),
  _pentry       (0),
  _config_buffer(0),
  _config_id    (Pds::TypeId::Any,0),
  _options      (0),
  _pedestals    (make_ndarray<unsigned>(1,1)),
  _pedestals_lo (make_ndarray<unsigned>(1,1)),
  _therm        (RDIV)
{
}

EpixHandler::~EpixHandler()
{
  if (_pentry)
    delete _pentry;
  if (_config_buffer)
    delete _config_buffer;
}

unsigned EpixHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* EpixHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void EpixHandler::rename(const char* s)
{
  if (_entry) {
    _entry->desc().name(s);
    char buff[64];
    int index=0;
    unsigned nAsics=0;
    bool lastRowExclusions=false;

#define PARSE_CONFIG(typ) 						\
      const typ& c = *reinterpret_cast<const typ*>(_config_buffer);	\
      nAsics = c.numberOfAsics();					

    switch(_config_id.id()) {
    case Pds::TypeId::Id_EpixConfig   : 
      { PARSE_CONFIG(Pds::Epix::ConfigV1);
	lastRowExclusions = c.lastRowExclusions(); } break;
    case Pds::TypeId::Id_Epix10kConfig: 
      { PARSE_CONFIG(Pds::Epix::Config10KV1);
	lastRowExclusions = c.lastRowExclusions(); } break;
    case Pds::TypeId::Id_Epix100aConfig: 
      { PARSE_CONFIG(Pds::Epix::Config100aV1);
	lastRowExclusions = true; } break;
    default:
      printf("EpixHandler::rename unrecognized configuration type %08x\n",
	     _config_id.value());
      return;
    }
    
#undef PARSE_CONFIG

    for(unsigned a=0; a<nAsics; a++) {
      sprintf(buff,"%s:AsicMonitor%d",s,a);
      _rename_cache(_feature[index++],buff);
    }
    if (lastRowExclusions) {
      sprintf(buff,"%s:Epix:AVDD",s);
      _rename_cache(_feature[index++],buff);
      sprintf(buff,"%s:Epix:DVDD",s);
      _rename_cache(_feature[index++],buff);
      sprintf(buff,"%s:Epix:AnaCardT",s);
      _rename_cache(_feature[index++],buff);
      sprintf(buff,"%s:Epix:StrBackT",s);
      _rename_cache(_feature[index++],buff);
      sprintf(buff,"%s:Epix:Humidity",s);
      _rename_cache(_feature[index++],buff);
    }
    if (Ami::EventHandler::post_diagnostics())
      for(unsigned a=0; a<16; a++) {
	sprintf(buff,"%s:CommonMode%d",s,_channel_map[a]);
	_rename_cache(_feature[index++],buff);
      }
  }
}

void EpixHandler::reset() 
{
  _entry = 0; 
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
  unsigned nchip_columns=1;
  unsigned nchip_rows   =1;
  unsigned columns = 0;
  unsigned rows    = 0;
  unsigned rowsPerAsic = 0;
  unsigned colsPerAsic = 0;
  unsigned aMask = 0;
  unsigned nAsics = 0;
  bool lastRowExclusions = true;

  if (tid.id()==Pds::TypeId::Id_GenericPgpConfig) {
    const Pds::GenericPgp::ConfigV1& c = *reinterpret_cast<const Pds::GenericPgp::ConfigV1*>(payload);
    reinterpret_cast<uint32_t&>(tid) = c.stream()[0].config_type();
    payload = reinterpret_cast<const void*>(c.payload().data()+c.stream()[0].config_offset());
  }

#define PARSE_CONFIG(typ) 						\
    const typ& c = *reinterpret_cast<const typ*>(payload);		\
    if (_config_buffer) delete[] _config_buffer;			\
    _config_buffer = new char[c._sizeof()];				\
    memcpy(_config_buffer,&c,c._sizeof());				\
    nchip_columns=c.numberOfAsicsPerRow();				\
    nchip_rows   =c.numberOfAsicsPerColumn();				\
    columns = c.numberOfColumns();					\
    rows    = c.numberOfRows();						\
    rowsPerAsic = c.numberOfRows()/c.numberOfAsicsPerColumn();		\
    colsPerAsic = c.numberOfPixelsPerAsicRow();				\
    aMask  = c.asicMask();						\
    nAsics = c.numberOfAsics();					       

  switch(tid.id()) {
  case Pds::TypeId::Id_EpixConfig   : 
    { PARSE_CONFIG(Pds::Epix::ConfigV1);
      lastRowExclusions = c.lastRowExclusions(); } break;
  case Pds::TypeId::Id_Epix10kConfig: 
    { PARSE_CONFIG(Pds::Epix::Config10KV1);
      lastRowExclusions = c.lastRowExclusions(); } break;
  case Pds::TypeId::Id_Epix100aConfig: 
    { PARSE_CONFIG(Pds::Epix::Config100aV1);
      lastRowExclusions = true; } break;
  case Pds::TypeId::Id_GenericPgpConfig:
    
  default:
    printf("EpixHandler::configure unrecognized configuration type %08x\n",
	   tid.value());
    return;
  }

#undef PARSE_CONFIG

  _config_id = tid;

  { int ppb = 1;
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, rows, ppb, ppb);

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
    
    _pentry = new EntryImage(desc);
    _pentry->invalid();
  }

  //
  //  Special case of one ASIC
  //     Make frame only as large as one ASIC
  //
  if ((aMask&(aMask-1))==0) {
    columns = colsPerAsic;
    rows    = rowsPerAsic;
    
    int ppb = image_ppbin(columns,rows);
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, rows, ppb, ppb);
    
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
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, rows, ppb, ppb);

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
  
  unsigned nFeatures = nAsics;
  if (lastRowExclusions)
    nFeatures += 5;
  if (Ami::EventHandler::post_diagnostics())
    nFeatures += 16;
  _feature = make_ndarray<int>(nFeatures);

  char buff[64];
  int index=0;
  for(unsigned a=0; a<nAsics; a++) {
    sprintf(buff,"%s:Epix:AsicMonitor:%d",detname,a);
    _feature[index++] = _add_to_cache(buff);
  }
  if (lastRowExclusions) {
    sprintf(buff,"%s:Epix:AVDD",detname);
    _feature[index++] = _add_to_cache(buff);
    sprintf(buff,"%s:Epix:DVDD",detname);
    _feature[index++] = _add_to_cache(buff);
    sprintf(buff,"%s:Epix:AnaCardT",detname);
    _feature[index++] = _add_to_cache(buff);
    sprintf(buff,"%s:Epix:StrBackT",detname);
    _feature[index++] = _add_to_cache(buff);
    sprintf(buff,"%s:Epix:Humidity",detname);
    _feature[index++] = _add_to_cache(buff);
  }
  if (Ami::EventHandler::post_diagnostics()) {
    for(unsigned a=0; a<16; a++) {
      sprintf(buff,"%s:Epix:CommonMode%d",detname,_channel_map[a]);
      _feature[index++] = _add_to_cache(buff);
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
    ndarray<const uint16_t,2> env;

    unsigned nchip_columns=1;
    unsigned nchip_rows   =1;
    unsigned rowsPerAsic = 0;
    unsigned colsPerAsic = 0;
    unsigned aMask = 0;
    unsigned nAsics = 0;
    unsigned lastRowExclusions = 0;

#define PARSE_CONFIG(typ,dtyp)  					\
      const dtyp& f = *reinterpret_cast<const dtyp*>(payload);		\
      const typ& c = *reinterpret_cast<const typ*>(_config_buffer);	\
      a = f.frame(c);							\
      temps = f.temperatures(c);					\
      nchip_columns=c.numberOfAsicsPerRow();				\
      nchip_rows   =c.numberOfAsicsPerColumn();				\
      rowsPerAsic = c.numberOfRows()/c.numberOfAsicsPerColumn();	\
      colsPerAsic = c.numberOfPixelsPerAsicRow();			\
      aMask  = c.asicMask();						\
      nAsics = c.numberOfAsics();					

    switch(_config_id.id()) {
    case Pds::TypeId::Id_EpixConfig   : 
      { PARSE_CONFIG(Pds::Epix::ConfigV1    ,Pds::Epix::ElementV1);
	env = f.excludedRows(c);
	lastRowExclusions = c.lastRowExclusions(); } break;
    case Pds::TypeId::Id_Epix10kConfig: 
      { PARSE_CONFIG(Pds::Epix::Config10KV1 ,Pds::Epix::ElementV1);
	env = f.excludedRows(c);
	lastRowExclusions = c.lastRowExclusions(); } break;
    case Pds::TypeId::Id_Epix100aConfig: 
      { PARSE_CONFIG(Pds::Epix::Config100aV1,Pds::Epix::ElementV2);
	env = f.environmentalRows(c);
	lastRowExclusions = true; } break;
    default:
      printf("EpixHandler::event unrecognized configuration type %08x\n",
	     _config_id.value());
      return;
    }

#undef PARSE_CONFIG

    _entry->reset();
    const DescImage& d = _entry->desc();

    const ndarray<const unsigned,2>& pa =
      d.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pedestals;

    const ndarray<const unsigned,2>& pa_lo =
      d.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pedestals_lo;

    int ppbin = _entry->desc().ppxbin();

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
	unsigned m = _asicLocation[asic].col;
	const uint16_t* d = & a[r][m*colsPerAsic];
	const unsigned* p_hi = &pa   [j][0];
	const unsigned* p_lo = &pa_lo[j][0];
        const unsigned* s = & _status[r][m*colsPerAsic];
	const SubFrame& fr = _desc.frame(0);
	for(unsigned k=0; k<colsPerAsic; k++) {
          unsigned v = s[k]==0 ? ((d[k]&0x4000) ? 
                                  unsigned(d[k]&0x3fff) + p_lo[k] :
                                  unsigned(d[k]&0x3fff) + p_hi[k]) :
            offset;
          _entry->addcontent(v, fr.x+k/ppbin, fr.y+j/ppbin);
	}
      }
    }
    else {
      for(unsigned i=0; i<nchip_rows; i++)
	for(unsigned j=0; j<rowsPerAsic; j++) {
	  unsigned r = i*rowsPerAsic+j;
	  for(unsigned m=0; m<nchip_columns; m++) {
	    unsigned fn = i*nchip_columns+m;
	    if (aMask & 1<<_asic_map[fn]) {
	      unsigned q = m*colsPerAsic;
	      const uint16_t* d    = &a    [r][q];
	      const unsigned* p_hi = &pa   [r][q];
	      const unsigned* p_lo = &pa_lo[r][q];
              const unsigned* s    = & _status[r][m*colsPerAsic];
	      const SubFrame& fr = _desc.frame(fn);
	      for(unsigned k=0; k<colsPerAsic; k++) {
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

    int index = 0;
#if 1
    for(unsigned ic=0; ic<nAsics; ic++)
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

    if (lastRowExclusions) {
      const uint16_t* last = &env[env.shape()[0]-1][0];
      _cache.cache(_feature[index++], double(last[2])*0.00183);
      _cache.cache(_feature[index++], double(last[3])*0.00183);
      _cache.cache(_feature[index++], double(last[4])*(-0.0194) + 78.393);
      _cache.cache(_feature[index++], _therm.getTemp(last[6]));
      _cache.cache(_feature[index++], double(last[7])*0.0291 - 23.8);
    }

    if (d.options()&FrameCalib::option_correct_common_mode2()) {
      for(unsigned k=0; k<_desc.nframes(); k++) {
        if ((aMask & 1<<_asic_map[k])==0) 
          continue;
        ndarray<uint32_t,2> e = _entry->contents(_desc.frame(k));
        unsigned shape[2];
        shape[0] = e.shape()[0];
        shape[1] = e.shape()[1]/4;
        for(unsigned m=0; m<4; m++) {
          for(unsigned y=0; y<shape[0]; y++) {
            ndarray<uint32_t,1> s(&e[y][m*shape[1]],&shape[1]);
            int fn = int(frameNoise(s,offset*int(d.ppxbin()*d.ppybin()+0.5)));
            if (Ami::EventHandler::post_diagnostics() && k==3 && m==0 && y<80)
              _cache.cache(_feature[(y%16)+index],double(fn));
            uint32_t* v = &s[0];
            for(unsigned x=0; x<shape[1]; x++)
              v[x] -= fn;
          }
        }
      }	
    }

    else if (d.options()&FrameCalib::option_correct_common_mode()) {
      for(unsigned k=0; k<_desc.nframes(); k++) {
        if ((aMask & 1<<_asic_map[k])==0) 
          continue;
        ndarray<uint32_t,2> e = _entry->contents(_desc.frame(k));
	unsigned shape[2];
	shape[0] = e.shape()[0];
	shape[1] = e.shape()[1]/4;
	for(unsigned m=0; m<4; m++) {
	  ndarray<uint32_t,2> s(&e[0][m*shape[1]],shape);
	  s.strides(e.strides());
          ndarray<uint32_t,2> t(_status.begin()+(s.begin()-_entry->content().begin()),shape);
          t.strides(_status.strides());
	  int fn = int(frameNoise(s,t,offset*int(d.ppxbin()*d.ppybin()+0.5)));
	  for(unsigned y=0; y<shape[0]; y++) {
	    uint32_t* v = &s[y][0];
	    for(unsigned x=0; x<shape[1]; x++)
	      v[x] -= fn;
	  }
	  if (Ami::EventHandler::post_diagnostics())
	    _cache.cache(_feature[4*k+m+index],double(fn));
	}
      }	
    }

    //
    //  Correct for gain
    //
    const ndarray<const double,2>& gn    = 
      (d.options()&FrameCalib::option_correct_gain()) ? _gain : _no_gain;
    const ndarray<const double,2>& gn_lo = 
      (d.options()&FrameCalib::option_correct_gain()) ? _gain_lo : _no_gain;
    {
      for(unsigned k=0; k<_desc.nframes(); k++) {
	if ((aMask & 1<<_asic_map[k])==0) 
	  continue;
	ndarray<uint32_t,2> e = _entry->contents(_desc.frame(k));
	unsigned r = (k/nchip_columns) * rowsPerAsic;
	unsigned m = (k%nchip_columns) * colsPerAsic;
	for(unsigned y=0; y<e.shape()[0]; y++,r++) {
	  uint32_t*        v = &e    [y][0];
	  const uint16_t*  d = &a    [r][m];
	  const double* g_hi = &gn   [r][m];
	  const double* g_lo = &gn_lo[r][m];
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
  
void EpixHandler::_damaged() { if (_entry) _entry->invalid(); }

void EpixHandler::_load_pedestals()
{
  unsigned rows=0, cols=0;

#define PARSE_CONFIG(typ) {					    \
    const typ& c = *reinterpret_cast<const typ*>(_config_buffer);   \
    rows = c.numberOfRows();					    \
    cols = c.numberOfColumns();					    \
  }

    
  switch(_config_id.id()) {
  case Pds::TypeId::Id_EpixConfig:
    PARSE_CONFIG(Pds::Epix::ConfigV1); break;
  case Pds::TypeId::Id_Epix10kConfig:
    PARSE_CONFIG(Pds::Epix::Config10KV1); break;
  case Pds::TypeId::Id_Epix100aConfig:
    PARSE_CONFIG(Pds::Epix::Config100aV1); break;
  default:
    return;
  }

#undef PARSE_CONFIG

  _status       = make_ndarray<unsigned>(rows,cols);
  _pedestals    = make_ndarray<unsigned>(rows,cols);
  _pedestals_lo = make_ndarray<unsigned>(rows,cols);
  _offset       = make_ndarray<unsigned>(rows,cols);
  for(unsigned* a = _offset.begin(); a!=_offset.end(); *a++ = offset) ;

  EntryImage* p = _pentry;
  if (FrameCalib::load_pedestals(p,0,"sta")) {

    printf("Loaded status %d x %d [%d x %d]\n",
           p->content().shape()[0],
           p->content().shape()[1],
           _status.shape()[0],
           _status.shape()[1]);

    for(unsigned *a=_status.begin(), *b=p->contents(); a!=_status.end(); *a++=*b++) ;

    const DescImage& d = _entry->desc();
    ImageMask mask(d.nbinsy(),d.nbinsx());
    mask.fill();
    for(unsigned i=0; i<_status.shape()[0]; i++)
      for(unsigned j=0; j<_status.shape()[1]; j++)
        if (_status[i][j])
          mask.clear(i,j);
    mask.update();
    _entry->desc().set_mask(mask);
  }
  else
    for(unsigned* a=_status.begin(); a!=_status.end(); *a++=0) ;

  if (FrameCalib::load_pedestals(p,offset,"ped")) {
    for(unsigned *a=_pedestals.begin(), *b=p->contents(); a!=_pedestals.end(); *a++=*b++) ;
  }
  else
    for(unsigned* a=_pedestals.begin(); a!=_pedestals.end(); *a++=offset) ;

  if (FrameCalib::load_pedestals(p,offset,"ped_lo")) {
    for(unsigned *a=_pedestals_lo.begin(), *b=p->contents(); a!=_pedestals_lo.end(); *a++=*b++) ;
  }
  else
    for(unsigned *a=_pedestals_lo.begin(), *b=_pedestals.begin(); a!=_pedestals_lo.end(); *a++=*b++) ;
}

void EpixHandler::_load_gains()
{
  unsigned rows=0, cols=0;

#define PARSE_CONFIG(typ) {					    \
    const typ& c = *reinterpret_cast<const typ*>(_config_buffer);   \
    rows = c.numberOfRows();					    \
    cols = c.numberOfColumns();					    \
  }

    
  switch(_config_id.id()) {
  case Pds::TypeId::Id_EpixConfig   :
    PARSE_CONFIG(Pds::Epix::ConfigV1); break;
  case Pds::TypeId::Id_Epix10kConfig:
    PARSE_CONFIG(Pds::Epix::Config10KV1); break;
  case Pds::TypeId::Id_Epix100aConfig:
    PARSE_CONFIG(Pds::Epix::Config100aV1); break;
  default:
    return;
  }

#undef PARSE_CONFIG

  _gain    = make_ndarray<double>(rows,cols);
  _gain_lo = make_ndarray<double>(rows,cols);
  _no_gain = make_ndarray<double>(rows,cols);

  for(double* a=_no_gain.begin(); a!=_no_gain.end(); *a++=1.) ;

  EntryImage* p = _pentry;
  ndarray<double,3> ap = FrameCalib::load(p->desc(),"gain");
  if (ap.size())
    for(double *a=_gain.begin(), *b=ap.begin(); a!=_gain.end(); *a++=*b++) ;
  else
    for(double* a=_gain.begin(); a!=_gain.end(); *a++=1.) ;

  ap = FrameCalib::load(p->desc(),"gain_lo");
  if (ap.size())
    for(double *a=_gain_lo.begin(), *b=ap.begin(); a!=_gain_lo.end(); *a++=*b++) ;
  else
    for(double* a=_gain_lo.begin(); a!=_gain_lo.end(); *a++=100.) ;
}

double _tps_temp(const uint16_t q)
{
  const double degCperADU = 1./28.1;
  const double t0 = -489.28;
  return double(q)*degCperADU + t0;
}


