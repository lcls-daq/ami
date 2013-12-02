#include "EpixHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/xtc/ClockTime.hh"


#include <string.h>

using namespace Ami;

static const unsigned offset=1<<16;

static const Pds::TypeId Config_Type = Pds::TypeId(Pds::TypeId::Id_EpixConfig,1);
static const Pds::TypeId Data_Type   = Pds::TypeId(Pds::TypeId::Id_EpixElement,1);

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

static double frameNoise(const ndarray<const uint32_t,2> data,
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


EpixHandler::EpixHandler(const Pds::Src& info, FeatureCache& cache) :
  //  EventHandler(info, Pds::TypeId::Id_EpixElement, Pds::TypeId::Id_EpixConfig),
  EventHandler  (info, Data_Type.id(), Config_Type.id()),
  _cache        (cache),
  _entry        (0),
  _config_buffer(0),
  _pedestals    (make_ndarray<unsigned>(1,1))
{
}

EpixHandler::~EpixHandler()
{
}

unsigned EpixHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* EpixHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void EpixHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void EpixHandler::reset() { _entry = 0; }

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

    const unsigned chip_margin=4;
    const unsigned nchip_columns=c.numberOfAsicsPerRow();
    const unsigned nchip_rows   =c.numberOfAsicsPerColumn();
    unsigned columns = nchip_columns*c.numberOfPixelsPerAsicRow() + (nchip_columns-1)*chip_margin;
    unsigned rows    = nchip_rows   *c.numberOfRowsPerAsic()      + (nchip_rows   -1)*chip_margin;
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

    _load_pedestals();
  }
}

void EpixHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

bool EpixHandler::used() const { return (_entry && _entry->desc().used()); }

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

    ndarray<const uint16_t,2> a = f.frame(_config);

    int ppbin = _entry->desc().ppxbin();
    {
      for(unsigned i=0; i<_config.numberOfAsicsPerColumn(); i++)
	for(unsigned j=0; j<_config.numberOfRowsPerAsic(); j++) {
	  unsigned r = i*_config.numberOfRowsPerAsic()+j;
	  const uint16_t* d = & a[r][0];
	  const unsigned* p = &pa[r][0];
	  for(unsigned m=0; m<_config.numberOfAsicsPerRow(); m++) {
	    const SubFrame& fr = _entry->desc().frame(i*_config.numberOfAsicsPerRow()+m);
	    for(unsigned k=0; k<_config.numberOfPixelsPerAsicRow(); k++)
	      _entry->addcontent(unsigned(*d++) + *p++, fr.x+k/ppbin, fr.y+j/ppbin);
	  }
	}
    }

    if (d.options()&FrameCalib::option_correct_common_mode()) {
      for(unsigned k=0; k<_entry->desc().nframes(); k++) {
	ndarray<uint32_t,2> a = _entry->contents(k);
	int fn = int(frameNoise(a,offset*int(d.ppxbin()*d.ppybin()+0.5)));
	for(uint32_t* it = a.begin(); it!=a.end(); it++)
	  *it -= fn;
      }	
    }

    _entry->info(double(offset*d.ppxbin()*d.ppybin()),EntryImage::Pedestal);
    _entry->info(1.,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void EpixHandler::_damaged() { _entry->invalid(); }

void EpixHandler::_load_pedestals()
{
  const Pds::Epix::ConfigV1& _config = *new(_config_buffer) Pds::Epix::ConfigV1;
  _pedestals = make_ndarray<unsigned>(_config.numberOfRows(),_config.numberOfColumns());
  _offset    = make_ndarray<unsigned>(_config.numberOfRows(),_config.numberOfColumns());
  for(unsigned* a = _offset.begin(); a!=_offset.end(); *a++ = offset) ;

  const unsigned ny = _config.numberOfRowsPerAsic();
  const unsigned nx = _config.numberOfPixelsPerAsicRow();

  const DescImage& d = _entry->desc();
  EntryImage* p = new EntryImage(d);
  if (FrameCalib::load_pedestals(p,offset)) {
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
}
