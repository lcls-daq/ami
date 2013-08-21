#include "EpixHandler.hh"

#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <string.h>

using namespace Ami;

static const unsigned offset=1<<16;

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


EpixHandler::EpixHandler(const Pds::Src& info) :
  EventHandler(info, Pds::TypeId::Any, Pds::TypeId::Any),
  _entry (0),
  _config(0,0,0,0,0),
  _pedestals(make_ndarray<unsigned>(1,1,1))
{
}

EpixHandler::~EpixHandler()
{
  if (_entry)
    delete _entry;
}

unsigned EpixHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* EpixHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void EpixHandler::reset() { _entry = 0; }

void EpixHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  if (tid.id()      == (Pds::TypeId::Type)Ami::Epix::ConfigT::typeId && 
      tid.version() == Ami::Epix::ConfigT::version) {
    if (_entry) 
      delete _entry;

    const Ami::Epix::ConfigT& c = *reinterpret_cast<const Ami::Epix::ConfigT*>(payload);
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    const unsigned chip_margin=4;
    unsigned columns = c.nchip_columns*c.ncolumns + (c.nchip_columns-1)*chip_margin;
    unsigned rows    = c.nchip_rows   *c.nrows    + (c.nchip_rows   -1)*chip_margin;
    unsigned pixels  = (columns > rows) ? columns : rows;
    unsigned ppb     = _full_resolution() ? 1 : (pixels-1)/640 + 1;
    columns = (columns+ppb-1)/ppb;
    rows    = (rows   +ppb-1)/ppb;

    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, rows, ppb, ppb);
    for(unsigned i=0; i<c.nchip_rows; i++)
      for(unsigned j=0; j<c.nchip_columns; j++)
	desc.add_frame(j*(c.ncolumns+chip_margin)/ppb,
		       i*(c.nrows   +chip_margin)/ppb,
		       c.ncolumns/ppb,
		       c.nrows   /ppb);

    _entry = new EntryImage(desc);
    _entry->invalid();

    _config = c;

    _load_pedestals(desc);
  }
}

void EpixHandler::_configure(const void* payload, const Pds::ClockTime& t) {}
void EpixHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}

#include "pdsdata/xtc/ClockTime.hh"

bool EpixHandler::used() const { return (_entry && _entry->desc().used()); }

void EpixHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  if (_entry && _entry->desc().used()) {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("EpixHandler::event options %x -> %x\n", _options, o);
      _options = o;
    }

    if (_entry->desc().options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals(_entry->desc());
      _entry->desc().options( _entry->desc().options()&~FrameCalib::option_reload_pedestal() );
    }

    const Ami::Epix::DataT& f = *reinterpret_cast<const Ami::Epix::DataT*>(payload);

    _entry->reset();
    const DescImage& d = _entry->desc();

    const uint16_t* p = reinterpret_cast<const uint16_t*>(&f+1);
    
    if (d.options()&FrameCalib::option_no_pedestal()) {
      for(unsigned i=0; i<_config.nrows; i++)
	for(unsigned j=0; j<_config.ncolumns; j++) {
	  unsigned fn=0;
	  for(unsigned k=0; k<_config.nchip_rows; k++)
	    for(unsigned m=0; m<_config.nchip_columns; m++,fn++) {
	      const SubFrame& f = _entry->desc().frame(fn);
	      _entry->addcontent(p[fn]+offset, f.x+j/d.ppxbin(), f.y+i/d.ppybin());
	    }
	  p += ((fn+1)&~1)*_config.nsamples;
	}
    
    }
    else {
      for(unsigned i=0; i<_config.nrows; i++)
	for(unsigned j=0; j<_config.ncolumns; j++) {
	  unsigned fn=0;
	  for(unsigned k=0; k<_config.nchip_rows; k++)
	    for(unsigned m=0; m<_config.nchip_columns; m++,fn++) {
	      const SubFrame& f = _entry->desc().frame(fn);
	      _entry->addcontent(p[fn]+offset-_pedestals[fn][k][m], f.x+j/d.ppxbin(), f.y+i/d.ppybin());
	    }
	  p += ((fn+1)&~1)*_config.nsamples;
	}
    }

    if (d.options()&FrameCalib::option_correct_common_mode()) {
      for(unsigned k=0; k<_entry->desc().nframes(); k++) {
	ndarray<uint32_t,2> a = _entry->contents(k);
	int fn = int(frameNoise(a,offset*d.ppxbin()*d.ppybin()));
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

void EpixHandler::_load_pedestals(const DescImage& desc)
{
  unsigned nf = desc.nframes();
  if (nf==0) nf=1;
  unsigned nx = _config.ncolumns;
  unsigned ny = _config.nrows;
  _pedestals = make_ndarray<unsigned>(nf,ny,nx);

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
	for (unsigned row=0; row < ny; row++) {
	  if (feof(f)) return;
	  getline(&linep, &sz, f);
	  _pedestals[s][row][0] = strtoul(linep,&pEnd,0);
	  for(unsigned col=1; col<nx; col++) 
	    _pedestals[s][row][col] = strtoul(pEnd, &pEnd,0);
	}
      }    
    }
      
    free(linep);
    fclose(f);
  }
  else {
    memset(&_pedestals[0][0][0],0,nf*nx*ny*sizeof(unsigned));
  }
}
