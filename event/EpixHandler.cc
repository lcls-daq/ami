#include "EpixHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/event/CspadTemp.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/xtc/ClockTime.hh"

#include <string.h>


using namespace Ami;

static const unsigned offset=1<<16;
static const double  doffset=double(offset);

static const Pds::TypeId Config_Type = Pds::TypeId(Pds::TypeId::Id_EpixConfig,1);
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
static double _thm_temp(const uint16_t q);

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
			 unsigned off)
{
  const int fnPixelMin = -100 + off;
  const int fnPixelMax =  100 + off;
  const int fnPixelBins = fnPixelMax - fnPixelMin;
  const int peakSpace   = 5;
  
  //  histogram the pixel values
  unsigned hist[fnPixelBins];
  { memset(hist, 0, fnPixelBins*sizeof(unsigned));
    for(unsigned i=0; i<data.shape()[0]; i++) {
      for(unsigned j=0; j<data.shape()[1]; j++) {
	int v = data[i][j] - fnPixelMin;
	if (v >= 0 && v < int(fnPixelBins))
	  hist[v]++;
      }
    }
  }

  double v = 0;
  // the first peak from the left above this is the pedestal
  { const int fnPeakBins = 5;
    const int fnPixelRange = fnPixelBins-fnPeakBins-1;
    const unsigned fnPedestalThreshold = 1000;
    
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
  //  EventHandler(info, Pds::TypeId::Id_EpixElement, Pds::TypeId::Id_EpixConfig),
  EventHandler  (info, Data_Type.id(), Config_Type.id()),
  _cache        (cache),
  _entry        (0),
  _pentry       (0),
  _config_buffer(0),
  _options      (0),
  _pedestals    (make_ndarray<unsigned>(1,1)),
  _pedestals_lo (make_ndarray<unsigned>(1,1))
{
}

EpixHandler::~EpixHandler()
{
  if (_pentry)
    delete _pentry;
}

unsigned EpixHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* EpixHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void EpixHandler::rename(const char* s)
{
  if (_entry) {
    _entry->desc().name(s);
    char buff[64];
    const Pds::Epix::ConfigV1& _config = *new(_config_buffer) Pds::Epix::ConfigV1;
    unsigned nAsics = _config.numberOfAsics();
    for(unsigned a=0; a<nAsics; a++) {
      sprintf(buff,"%s:TempRelAsic%d",s,a);
      _cache.rename(_feature[a],buff);
    }
    if (Ami::EventHandler::post_diagnostics())
      for(unsigned a=0; a<16; a++) {
	sprintf(buff,"%s:CommonMode%d",s,_channel_map[a]);
	_cache.rename(_feature[a+_config.numberOfAsics()],buff);
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
//  Start simple, assume a single readout tile
//
void EpixHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  if (tid.value() == Config_Type.value()) {
    const Pds::Epix::ConfigV1& c = *reinterpret_cast<const Pds::Epix::ConfigV1*>(payload);
    if (_config_buffer) delete[] _config_buffer;
    _config_buffer = new char[c._sizeof()];
    memcpy(_config_buffer,&c,c._sizeof());

    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    const char* detname = Pds::DetInfo::name(det.detector());

    const unsigned chip_margin=4;
    unsigned nchip_columns=c.numberOfAsicsPerRow();
    unsigned nchip_rows   =c.numberOfAsicsPerColumn();

    unsigned columns = nchip_columns*c.numberOfPixelsPerAsicRow() + (nchip_columns-1)*chip_margin;
    unsigned rows    = nchip_rows   *c.numberOfRowsPerAsic()      + (nchip_rows   -1)*chip_margin;

    { int ppb = 1;
      DescImage desc(det, (unsigned)0, ChannelID::name(det),
		     columns, rows, ppb, ppb);
      for(unsigned i=0; i<nchip_rows; i++)
	for(unsigned j=0; j<nchip_columns; j++) {
	  float x0 = j*(c.numberOfPixelsPerAsicRow()+chip_margin);
	  float y0 = i*(c.numberOfRowsPerAsic()     +chip_margin);
	  float x1 = x0+c.numberOfPixelsPerAsicRow();
	  float y1 = y0+c.numberOfRowsPerAsic();
	  desc.add_frame(desc.xbin(x0),desc.ybin(y0),
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
    unsigned aMask = c.asicMask();
    if ((aMask&(aMask-1))==0) {
      columns = c.numberOfPixelsPerAsicRow();
      rows    = c.numberOfRowsPerAsic()     ;

      int ppb = image_ppbin(columns,rows);
      DescImage desc(det, (unsigned)0, ChannelID::name(det),
		     columns, rows, ppb, ppb);
      float x0 = 0;
      float y0 = 0;
      float x1 = x0+c.numberOfPixelsPerAsicRow();
      float y1 = y0+c.numberOfRowsPerAsic();
      desc.add_frame(desc.xbin(x0),desc.ybin(y0),
		     desc.xbin(x1)-desc.xbin(x0),
		     desc.ybin(y1)-desc.ybin(y0));
      
      _entry = new EntryImage(desc);
      _entry->invalid();
    }
    else {
      int ppb = image_ppbin(columns,rows);
      DescImage desc(det, (unsigned)0, ChannelID::name(det),
		     columns, rows, ppb, ppb);
      for(unsigned i=0; i<nchip_rows; i++)
	for(unsigned j=0; j<nchip_columns; j++) {
	  float x0 = j*(c.numberOfPixelsPerAsicRow()+chip_margin);
	  float y0 = i*(c.numberOfRowsPerAsic()     +chip_margin);
	  float x1 = x0+c.numberOfPixelsPerAsicRow();
	  float y1 = y0+c.numberOfRowsPerAsic();
	  desc.add_frame(desc.xbin(x0),desc.ybin(y0),
			 desc.xbin(x1)-desc.xbin(x0),
			 desc.ybin(y1)-desc.ybin(y0));
	}
      
      _entry = new EntryImage(desc);
      _entry->invalid();
    }

    unsigned nFeatures = 4;
    if (Ami::EventHandler::post_diagnostics())
      nFeatures += 16;
    _feature = make_ndarray<int>(nFeatures);

    char buff[64];
    for(unsigned a=0; a<c.numberOfAsics(); a++) {
      sprintf(buff,"%s:Epix:TempRelAsic%d",detname,a);
      _feature[a] = _cache.add(buff);
    }
    if (Ami::EventHandler::post_diagnostics()) {
      for(unsigned a=0; a<16; a++) {
	sprintf(buff,"%s:Epix:CommonMode%d",detname,_channel_map[a]);
	_feature[a+c.numberOfAsics()] = _cache.add(buff);
      }
    }

    _load_pedestals();
    _load_gains    ();
  }
}

void EpixHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

bool EpixHandler::used() const 
{ 
  if (_entry && _entry->desc().used()) return true;
  for(unsigned i=0; i<_feature.shape()[0]; i++)
    if (_cache.used(_feature[i])) return true;
  return false;
}

void EpixHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  if (_entry && _entry->desc().used()) {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("EpixHandler::event options %x -> %x\n", _options, o);
      _options = o;
    }

    if (_entry->desc().options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( _entry->desc().options()&~FrameCalib::option_reload_pedestal() );
    }

    const Pds::Epix::ElementV1& f = *reinterpret_cast<const Pds::Epix::ElementV1*>(payload);
    const Pds::Epix::ConfigV1& _config = *new(_config_buffer) Pds::Epix::ConfigV1;

    _entry->reset();
    const DescImage& d = _entry->desc();

    const ndarray<const unsigned,2>& pa =
      d.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pedestals;

    const ndarray<const unsigned,2>& pa_lo =
      d.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pedestals_lo;

    ndarray<const uint16_t,2> a = f.frame(_config);

    int ppbin = _entry->desc().ppxbin();

    unsigned aMask = _config.asicMask();
    //
    //  Special case of one ASIC
    //
    if ((aMask&(aMask-1))==0) {
      unsigned asic=0;
      while((aMask&(1<<asic))==0)
	asic++;
      unsigned i = _asicLocation[asic].row;
      for(unsigned j=0; j<_config.numberOfRowsPerAsic(); j++) {
	unsigned r = i*_config.numberOfRowsPerAsic()+j;
	unsigned m = _asicLocation[asic].col;
	const uint16_t* d = & a[r][m*_config.numberOfPixelsPerAsicRow()];
	const unsigned* p_hi = &pa   [j][0];
	const unsigned* p_lo = &pa_lo[j][0];
	const SubFrame& fr = _entry->desc().frame(0);
	for(unsigned k=0; k<_config.numberOfPixelsPerAsicRow(); k++) {
	  unsigned v = (d[k]&0x4000) ? 
	    unsigned(d[k]&0x3fff) + p_lo[k] :
	    unsigned(d[k]&0x3fff) + p_hi[k];
	  _entry->addcontent(v, fr.x+k/ppbin, fr.y+j/ppbin);
	}
      }
    }
    else {
      for(unsigned i=0; i<_config.numberOfAsicsPerColumn(); i++)
	for(unsigned j=0; j<_config.numberOfRowsPerAsic(); j++) {
	  unsigned r = i*_config.numberOfRowsPerAsic()+j;
	  for(unsigned m=0; m<_config.numberOfAsicsPerRow(); m++) {
	    unsigned fn = i*_config.numberOfAsicsPerRow()+m;
	    if (_config.asicMask() & 1<<_asic_map[fn]) {
	      unsigned q = m*_config.numberOfPixelsPerAsicRow();
	      const uint16_t* d    = &a    [r][q];
	      const unsigned* p_hi = &pa   [r][q];
	      const unsigned* p_lo = &pa_lo[r][q];
	      const SubFrame& fr = _entry->desc().frame(fn);
	      for(unsigned k=0; k<_config.numberOfPixelsPerAsicRow(); k++) {
		unsigned v = (d[k]&0x4000) ? 
		  unsigned(d[k]&0x3fff) + p_lo[k] :
		  unsigned(d[k]&0x3fff) + p_hi[k];
		_entry->addcontent(v, fr.x+k/ppbin, fr.y+j/ppbin);
	      }
	    }
	  }
	}
    }

    ndarray<const uint16_t,1> temps = f.temperatures(_config);
#if 0
    for(unsigned a=0; a<_config.numberOfAsics(); a++)
      _cache.cache(_feature[a],_tps_temp(temps[a]));
#else
    if (aMask&1)
      _cache.cache(_feature[0],_tps_temp(temps[0]));
    if (aMask&2)
      _cache.cache(_feature[1],_thm_temp(temps[1]));
    if (aMask&4)
      _cache.cache(_feature[2],_thm_temp(temps[2]));
    if (aMask&8)
      _cache.cache(_feature[3],_tps_temp(temps[3]));
#endif

    if (d.options()&FrameCalib::option_correct_common_mode2()) {
      for(unsigned k=0; k<_entry->desc().nframes(); k++) {
	if ((_config.asicMask() & 1<<_asic_map[k])==0) 
	  continue;
	ndarray<uint32_t,2> e = _entry->contents(k);
	unsigned shape[2];
	shape[0] = e.shape()[0];
	shape[1] = e.shape()[1]/4;
	for(unsigned m=0; m<4; m++) {
	  for(unsigned y=0; y<shape[0]; y++) {
	    ndarray<uint32_t,1> s(&e[y][m*shape[1]],&shape[1]);
	    int fn = int(frameNoise(s,offset*int(d.ppxbin()*d.ppybin()+0.5)));
	    if (Ami::EventHandler::post_diagnostics() && k==3 && m==0 && y<80)
	      _cache.cache(_feature[(y%16)+_config.numberOfAsics()],double(fn));
	    uint32_t* v = &s[0];
	    for(unsigned x=0; x<shape[1]; x++)
	      v[x] -= fn;
	  }
	}
      }	
    }

    else if (d.options()&FrameCalib::option_correct_common_mode()) {
      for(unsigned k=0; k<_entry->desc().nframes(); k++) {
	if ((_config.asicMask() & 1<<_asic_map[k])==0) 
	  continue;
	ndarray<uint32_t,2> e = _entry->contents(k);
	unsigned shape[2];
	shape[0] = e.shape()[0];
	shape[1] = e.shape()[1]/4;
	for(unsigned m=0; m<4; m++) {
	  ndarray<uint32_t,2> s(&e[0][m*shape[1]],shape);
	  s.strides(e.strides());
	  int fn = int(frameNoise(s,offset*int(d.ppxbin()*d.ppybin()+0.5)));
	  for(unsigned y=0; y<shape[0]; y++) {
	    uint32_t* v = &s[y][0];
	    for(unsigned x=0; x<shape[1]; x++)
	      v[x] -= fn;
	  }
	  if (Ami::EventHandler::post_diagnostics())
	    _cache.cache(_feature[4*k+m+_config.numberOfAsics()],double(fn));
	}
      }	
    }

    //
    //  Correct for gain
    //
    const ndarray<const double,2>& gn    = _gain;
    const ndarray<const double,2>& gn_lo = _gain_lo;
    {
      for(unsigned k=0; k<_entry->desc().nframes(); k++) {
	if ((_config.asicMask() & 1<<_asic_map[k])==0) 
	  continue;
	ndarray<uint32_t,2> e = _entry->contents(k);
	unsigned r = (k/_config.numberOfAsicsPerRow()) * _config.numberOfRowsPerAsic();
	unsigned m = (k%_config.numberOfAsicsPerRow()) * _config.numberOfPixelsPerAsicRow();
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
  const Pds::Epix::ConfigV1& _config = *new(_config_buffer) Pds::Epix::ConfigV1;
  _pedestals    = make_ndarray<unsigned>(_config.numberOfRows(),_config.numberOfColumns());
  _pedestals_lo = make_ndarray<unsigned>(_config.numberOfRows(),_config.numberOfColumns());
  _offset       = make_ndarray<unsigned>(_config.numberOfRows(),_config.numberOfColumns());
  for(unsigned* a = _offset.begin(); a!=_offset.end(); *a++ = offset) ;

  const unsigned ny = _config.numberOfRowsPerAsic();
  const unsigned nx = _config.numberOfPixelsPerAsicRow();

  EntryImage* p = _pentry;
  if (FrameCalib::load_pedestals(p,offset,"ped")) {
    for(unsigned i=0; i<_config.numberOfAsicsPerColumn(); i++)
      for(unsigned j=0; j<_config.numberOfAsicsPerRow(); j++) {
	ndarray<uint32_t,2> ap = p->contents(i*_config.numberOfAsicsPerRow()+j);
	for(unsigned k=0; k<ny; k++)
	  for(unsigned m=0; m<nx; m++)
	    _pedestals[i*ny+k][j*nx+m] = ap[k][m];
      }
  }
  else
    for(unsigned* a=_pedestals.begin(); a!=_pedestals.end(); *a++=offset) ;

  if (FrameCalib::load_pedestals(p,offset,"ped_lo")) {
    for(unsigned i=0; i<_config.numberOfAsicsPerColumn(); i++)
      for(unsigned j=0; j<_config.numberOfAsicsPerRow(); j++) {
	ndarray<uint32_t,2> ap = p->contents(i*_config.numberOfAsicsPerRow()+j);
	for(unsigned k=0; k<ny; k++)
	  for(unsigned m=0; m<nx; m++)
	    _pedestals_lo[i*ny+k][j*nx+m] = ap[k][m];
      }
  }
  else
    for(unsigned *a=_pedestals_lo.begin(), *b=_pedestals.begin(); a!=_pedestals_lo.end(); *a++=*b++) ;
}

void EpixHandler::_load_gains()
{
  const Pds::Epix::ConfigV1& _config = *new(_config_buffer) Pds::Epix::ConfigV1;
  _gain    = make_ndarray<double>(_config.numberOfRows(),_config.numberOfColumns());
  _gain_lo = make_ndarray<double>(_config.numberOfRows(),_config.numberOfColumns());

  const unsigned ny = _config.numberOfRowsPerAsic();
  const unsigned nx = _config.numberOfPixelsPerAsicRow();

  EntryImage* p = _pentry;
  ndarray<double,3> ap = FrameCalib::load(p->desc(),"gain");
  if (ap.size()) {
    for(unsigned i=0,q=0; i<_config.numberOfAsicsPerColumn(); i++)
      for(unsigned j=0; j<_config.numberOfAsicsPerRow(); j++,q++)
	for(unsigned k=0; k<ny; k++)
	  for(unsigned m=0; m<nx; m++)
	    _gain[i*ny+k][j*nx+m] = ap[q][k][m];
  }
  else
    for(double* a=_gain.begin(); a!=_gain.end(); *a++=1.) ;

  ap = FrameCalib::load(p->desc(),"gain_lo");
  if (ap.size()) {
    for(unsigned i=0,q=0; i<_config.numberOfAsicsPerColumn(); i++)
      for(unsigned j=0; j<_config.numberOfAsicsPerRow(); j++,q++)
	for(unsigned k=0; k<ny; k++)
	  for(unsigned m=0; m<nx; m++)
	    _gain_lo[i*ny+k][j*nx+m] = ap[q][k][m];
  }
  else
    for(double* a=_gain_lo.begin(); a!=_gain_lo.end(); *a++=100.) ;
}

double _tps_temp(const uint16_t q)
{
  const double degCperADU = 1./28.1;
  const double t0 = -489.28;
  return double(q)*degCperADU + t0;
}

double _thm_temp(const uint16_t q)
{
  return CspadTemp::instance().getTemp(q);
}
