#include "CspadMiniHandler.hh"
#include "CspadAlignment.hh"

#include "ami/event/CspadTemp.hh"
#include "ami/event/CspadCalib.hh"
#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/DescImage.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/PeakFinder.hh"
#include "ami/data/PeakFinderFn.hh"

#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/psddl/cspad2x2.ddl.h"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#define UNBINNED
#define DO_PED_CORR
#define POST_INTEGRAL

using Ami::CspadCalib;
using namespace Pds;

typedef Pds::CsPad2x2::ElementV1 MiniElement;

static const unsigned Offset = 0x4000;
static const double  dOffset = double(Offset);
static const double pixel_size = 110;
static const unsigned Columns = CsPad2x2::ColumnsPerASIC;
static const unsigned Rows    = CsPad2x2::MaxRowsPerASIC*2;
static const float HI_GAIN_F = 1.;
static const float LO_GAIN_F = 7.;
static const double RDIV = 20000;

static inline unsigned sum1(const int16_t*& data,
                            const int16_t*& off,
                            const int16_t* const*& psp,
                            const double fn,
                            const float*& gn)
{ 
  unsigned v;
  if (off==*psp) { psp++; v = Offset; }
  else { 
    double d = (*gn)*(double(*data + *off - Offset) - fn);
    d += Offset; 
    v = unsigned(d+0.5);
  }
  off++;
  gn++;
  return v;
}

static const unsigned no_threshold = 0x00ffffff;

static inline unsigned thr1(double v0, double v1,
                            const int16_t*& off,
                            const int16_t* const*& psp,
                            const float*& rms)
{ 
  unsigned v;
  if (off==*psp) { psp++; v = no_threshold; }
  else {  v = unsigned(v0 + *rms*v1 + 0.5); }
  off++;
  rms++;
  return v;
}


static double frameNoise(const int16_t*  data,
                         const int16_t*  off,
                         const int16_t* const* sta)
{
  const unsigned ColBins = Columns;
  const unsigned RowBins = Rows;
  const int fnPixelMin = -100 + Offset;
  const int fnPixelMax =  100 + Offset;
  const int fnPixelBins = fnPixelMax - fnPixelMin;
  const int peakSpace   = 5;
  
  //  histogram the pixel values
  unsigned hist[fnPixelBins];
  { memset(hist, 0, fnPixelBins*sizeof(unsigned));
    const int16_t* d(data);
    const int16_t* o(off );
    for(unsigned i=0; i<ColBins; i++) {
      for(unsigned j=0; j<RowBins; j++, d+=2, o++) {
        if (*sta == o)
          sta++;
        else {
          int v = *d + *o - fnPixelMin;
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
      for(unsigned j=i-fnPeakBins+1; j<i+fnPeakBins; j++) {
        s0 += hist[j];
        s1 += hist[j]*j;
      }
      
      double binMean = double(s1)/double(s0);
      v =  binMean + fnPixelMin - Offset;
      
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

static double unbondedNoise(const int16_t*  data,
                            const int16_t*  off)
{
#define CORR(i) (data[i*2]+off[i])
  const unsigned ColBins = CsPad::ColumnsPerASIC;
  const unsigned RowBins = CsPad::MaxRowsPerASIC;
  ndarray<uint16_t,1> a = make_ndarray<uint16_t>(21);
  a[0] = CORR(RowBins-1);
  a[1] = CORR(2*RowBins*(ColBins-1));
  a[2] = CORR(2*RowBins*(ColBins-1)+RowBins-1);
  for(unsigned i=0,j=0; i<18; i++, j+=20*RowBins+10) {
    a[i+3] = CORR(j);
  }
  unsigned lo = Offset-100;
  unsigned hi = Offset+100;
  int v = Ami::FrameCalib::median(a,lo,hi);
  return double(v)-dOffset;
}

namespace Ami {
namespace CspadMiniGeometry {

  //
  //  When filling the image, compensate data which
  //    only partially fills a pixel (at the edges)
  //
#define FRAME_BOUNDS 							\
  const unsigned ColLen   = Columns/ppb-1;                              \
    const unsigned RowLen = Rows/ppb-1;                                 \
    unsigned x0 = CALC_X(column,0,0);					\
    unsigned x1 = CALC_X(column,ColLen,RowLen);                         \
    unsigned y0 = CALC_Y(row,0,0);					\
    unsigned y1 = CALC_Y(row,ColLen,RowLen);				\
    if (x0 > x1) { unsigned t=x0; x0=x1; x1=t; }			\
    if (y0 > y1) { unsigned t=y0; y0=y1; y1=t; }			


#define BIN_ITER1(F1) {							\
    const unsigned ColBins = Columns;                                   \
    const unsigned RowBins = Rows>>1;                                   \
    /*  fill the target region  */					\
    for(unsigned i=0; i<ColBins; i++) {					\
      unsigned j=0; double fn=fn1;                                      \
      while(j < RowBins) {                                              \
	const unsigned x = CALC_X(column,i,j);				\
	const unsigned y = CALC_Y(row   ,i,j);				\
	image.content(F1,x,y);                                          \
        j++;                                                            \
        data+=2;                                                        \
      }									\
      fn = fn2;                                                         \
      while(j < RowBins*2) {                                            \
	const unsigned x = CALC_X(column,i,j);				\
	const unsigned y = CALC_Y(row   ,i,j);				\
	image.content(F1,x,y);                                          \
        j++;                                                            \
        data+=2;                                                        \
      }									\
    }									\
  }

  //
  //  This class locates the ASIC data to the binned image grid
  //
  class Asic {
  public:
    Asic(double x, double y, unsigned ppbin) :
      column(unsigned( x/pixel_size)/ppbin),
      row   (unsigned(-y/pixel_size)/ppbin),
      ppb(ppbin) 
    {}
    virtual ~Asic() {}
  public:
    virtual void fill(Ami::DescImage& image) const = 0;
    virtual void fill(Ami::EntryImage& image,
		      double, double) const = 0;
    virtual void fill(Ami::EntryImage&   image,
		      const int16_t*     data,
                      Ami::FeatureCache& cache,
                      unsigned           index) const = 0;
    virtual void set_pedestals(double*) { printf("v set_pedestals\n"); }
  public:
    virtual void boundary(unsigned& x0, unsigned& x1, 
			  unsigned& y0, unsigned& y1) const = 0;
  protected:
    unsigned column, row;
    unsigned ppb;
    int16_t*  _sta[Columns*Rows];
  };

  class AsicP : public Asic {
  public:
    AsicP(double x, double y, unsigned ppbin, 
          double* ped, double* status, double* gain, double* sigma,
          const ndarray<const uint16_t,2>& gmap, unsigned imap) :
      Asic(x,y,ppbin)
    { // load offset-pedestal 
      if (ped) {
        int16_t* off = _off;
        for(unsigned col=0; col<Columns; col++) {
          for (unsigned row=0; row < Rows; row++)
            *off++ = Offset - int16_t(*ped++);
        }
      }
      else {
        int16_t* off = _off;
        for(unsigned col=0; col<Columns; col++)
          for (unsigned row=0; row < Rows; row++)
            *off++ = Offset;
      }

      if (status) {
        int16_t*  off = _off;
        int16_t** sta = _sta;
        for(unsigned col=0; col<Columns; col++) {
          for (unsigned row=0; row < Rows; row++, off++)
            if (*status++) *sta++ = off;
        }
      }
      else
        _sta[0] = 0;

      if (gain) {
        float* gn = _gn;
        for(unsigned col=0; col<Columns; col++)
          for (unsigned row=0; row<Rows; row++)
            *gn++ = *gain++;
      }
      else {
        float* gn = _gn;
        for(unsigned col=0; col<Columns; col++)
          for (unsigned row=0; row<Rows; row++)
            *gn++ = 1.;
      }
      
      { float* gn = _gn;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          for (unsigned row=0; row < Pds::CsPad::MaxRowsPerASIC; row++)
            *gn++ *= ((gmap(col,row)>>imap)&1) ? HI_GAIN_F:LO_GAIN_F;
          for (unsigned row=0; row < Pds::CsPad::MaxRowsPerASIC; row++)
            *gn++ *= ((gmap(col,row)>>imap)&2) ? HI_GAIN_F:LO_GAIN_F;
        }
      }

      if (sigma) {
        float* sg = _sg;
        float* gn = _gn;
        for(unsigned col=0; col<Columns; col++) {
          for (unsigned row=0; row<Rows; row++)
            *sg++ = *sigma++*(*gn++);
        }
      }
      else {
        float* sg = _sg;
        for(unsigned col=0; col<Columns; col++) {
          for (unsigned row=0; row<Rows; row++)
            *sg++ = 0;
        }
      }
    }
    void set_pedestals(double* ped)
    {
      if (ped) {
        int16_t* off = _off;
        for(unsigned col=0; col<Columns; col++)
          for (unsigned row=0; row<Rows; row++)
            *off++ = Offset - int16_t(*ped++);
      }
      else {
        int16_t* off = _off;
        for(unsigned col=0; col<Columns; col++)
          for (unsigned row=0; row<Rows; row++)
            *off++ = Offset;
      }
    }
  protected:
    int16_t  _off [Columns*Rows];
    float     _gn [Columns*Rows];
    float     _sg [Columns*Rows];
  };

  static int16_t  off_no_ped[Columns*Rows];
  static float    fgn_no_ped[Columns*Rows];

#define AsicTemplate(classname,bi,ti,PPB,rot)                           \
  class classname : public AsicP {					\
  public:								\
    classname(double x, double y,                                       \
              double* p, double* s, double* g, double* r,               \
              const ndarray<const uint16_t,2>& gmap, unsigned imap)     \
      : AsicP(x,y,PPB,p,s,g,r,gmap,imap) {}                             \
    void boundary(unsigned& dx0, unsigned& dx1,				\
		  unsigned& dy0, unsigned& dy1) const {			\
      FRAME_BOUNDS;							\
      dx0=x0; dx1=x1; dy0=y0; dy1=y1; }					\
    void fill(Ami::DescImage& image) const {				\
      FRAME_BOUNDS;							\
      image.add_frame(x0,y0,x1-x0+1,y1-y0+1,rot);                       \
    }									\
    void fill(Ami::EntryImage& image,                                   \
              double v0, double v1) const {                             \
      unsigned data = 0;                                                \
      bool lsuppress  = image.desc().options()&CspadCalib::option_suppress_bad_pixels(); \
      int16_t* zero = 0;                                               \
      const int16_t* off = _off;                                       \
      const int16_t* const * sta = lsuppress ? _sta : &zero;           \
      const float* rms = _sg;                                           \
      double fn1=0,fn2=0;                                               \
      ti;                                                               \
    }									\
    void fill(Ami::EntryImage&   image,					\
	      const int16_t*     data,                                  \
              Ami::FeatureCache& cache,                                 \
              unsigned           index) const {                         \
      bool lsuppress  = image.desc().options()&CspadCalib::option_suppress_bad_pixels(); \
      bool lcorrectfn = image.desc().options()&CspadCalib::option_correct_common_mode(); \
      bool lcorrectun = image.desc().options()&CspadCalib::option_correct_unbonded(); \
      bool lcorrectgn = image.desc().options()&CspadCalib::option_correct_gain(); \
      bool lnopedestal= image.desc().options()&CspadCalib::option_no_pedestal(); \
      int16_t* zero = 0;                                               \
      const int16_t*  off = lnopedestal ? off_no_ped : _off;           \
      const int16_t* const * sta = lsuppress ? _sta : &zero;           \
      const float* gn = (lnopedestal || !lcorrectgn) ? fgn_no_ped :_gn; \
      double fn1=0,fn2=0;                                               \
      if (lcorrectfn) { fn1 = fn2 = frameNoise(data,off,sta); }         \
      else if (lcorrectun) {                                            \
        fn1 = unbondedNoise(data,off);                                  \
        fn2 = unbondedNoise(data+CsPad::MaxRowsPerASIC*2,               \
                            off +CsPad::MaxRowsPerASIC);                \
      }                                                                 \
      if (Ami::EventHandler::post_diagnostics()) {                      \
        cache.cache(index,fn1);                                         \
        cache.cache(index+1,fn2);                                       \
      }                                                                 \
      bi;                                                               \
    }                                                                   \
  }

#define B1 { BIN_ITER1(sum1(data,off,sta,fn,gn)); }
#define T1 { BIN_ITER1(thr1(v0,v1,off,sta,rms)); }

#define CALC_X(a,b,c) (a+b)			    
#define CALC_Y(a,b,c) (a-c)			     
  AsicTemplate(  AsicD0B1P, B1, T1, 1, D0);
#undef CALC_X
#undef CALC_Y
#define CALC_X(a,b,c) (a+c)			    
#define CALC_Y(a,b,c) (a+b)			     
  AsicTemplate( AsicD90B1P, B1, T1, 1, D90);
#undef CALC_X
#undef CALC_Y
#define CALC_X(a,b,c) (a-b)			    
#define CALC_Y(a,b,c) (a+c)			     
  AsicTemplate(AsicD180B1P, B1, T1, 1, D180);
#undef CALC_X
#undef CALC_Y
#define CALC_X(a,b,c) (a-c)			    
#define CALC_Y(a,b,c) (a-b)			     
  AsicTemplate(AsicD270B1P, B1, T1, 1, D270);
#undef CALC_X
#undef CALC_Y

#undef B1
#undef T1
#undef AsicTemplate

  class TwoByTwo {
  public:
    TwoByTwo(double x, double y, unsigned ppb,
	     const Ami::Cspad::TwoByOneAlignment* a,
             const ndarray<const uint16_t,2>& gmap, 
             double* f=0, double* s=0, double* g=0, double* rms=0) 
    {
      for(unsigned i=0,imap=0; i<2; i++,imap+=2) {
        //  rotate in place
	double tx = a[i]._pad.x + x;
	double ty = a[i]._pad.y + y;

        switch(a[i]._rot) {
        case D0  : asic[i] = new  AsicD0B1P  (tx,ty,f,s,g,rms,gmap,imap); break;
        case D90 : asic[i] = new  AsicD90B1P (tx,ty,f,s,g,rms,gmap,imap); break;
        case D180: asic[i] = new  AsicD180B1P(tx,ty,f,s,g,rms,gmap,imap); break;
        case D270: asic[i] = new  AsicD270B1P(tx,ty,f,s,g,rms,gmap,imap); break;
        default  : break;
        }

        if (f) f+=Columns*Rows;
        if (s) s+=Columns*Rows;
        if (g) g+=Columns*Rows;
        if (rms) rms+=Columns*Rows;
      }
    }
    ~TwoByTwo() {  for(unsigned i=0; i<2; i++) delete asic[i]; }
    void fill(Ami::DescImage& image,
              unsigned        mask) const
    {
      if (mask&1) asic[0]->fill(image);
      if (mask&2) asic[1]->fill(image);
    }
    void fill(Ami::EntryImage& image,
              unsigned mask,
              double v0, double v1) const
    {
      if (mask&1) asic[0]->fill(image,v0,v1);
      if (mask&2) asic[1]->fill(image,v0,v1);
    }
    void fill(Ami::EntryImage&                image,
	      const ndarray<const int16_t,3>& element,
              Ami::FeatureCache&              cache,
              unsigned                        index) const
    {
      asic[0]->fill(image,&element(0,0,0),cache,index+0);
      asic[1]->fill(image,&element(0,0,1),cache,index+2);
    }
    void set_pedestals(double* f)
    {
      asic[0]->set_pedestals(f);
      asic[1]->set_pedestals(f ? f+Columns*Rows:0);
    }
  public:
    Asic* asic[2];
  };

  class ConfigCache {
  public:
    ConfigCache(Pds::TypeId type, const void* payload) : 
      _type(type)
    {
      _gainMap = make_ndarray<uint16_t>(CsPad::ColumnsPerASIC,
                                        CsPad::MaxRowsPerASIC);

      unsigned size=0;
      switch(type.id()) {
      case Pds::TypeId::Id_CspadConfig:
        switch(type.version()) {
#define CASE_VSN(v) case v:                                             \
          { const Pds::CsPad::ConfigV##v& c =                           \
              *reinterpret_cast<const Pds::CsPad::ConfigV##v*>(payload); \
            size = sizeof(c);                                           \
            _gainMap = c.quads(0).gm().gainMap().copy();                \
            break; }
          
          CASE_VSN(2)
          CASE_VSN(3)
          CASE_VSN(4)
          CASE_VSN(5)

#undef CASE_VSN
          default:
          break;
        } break;
      case Pds::TypeId::Id_Cspad2x2Config:
        switch(type.version()) {

#define CASE_VSN(v) case v:                                             \
          { const Pds::CsPad2x2::ConfigV##v& c =                        \
              *reinterpret_cast<const Pds::CsPad2x2::ConfigV##v*>(payload); \
            size = sizeof(c);                                           \
            _gainMap = c.quad().gm().gainMap().copy();                  \
            break; }

          CASE_VSN(1)
          CASE_VSN(2)

#undef CASE_VSN
          default:
          break;
        } break;
      default:
        break;
      }

      _payload = new char[size];
      memcpy(_payload,payload,size);
    }
    ConfigCache(const ConfigCache& c) : _type(c._type)
    { 
      unsigned size=0;
      switch(_type.id()) {
      case Pds::TypeId::Id_CspadConfig:
        switch(_type.version()) {
        case 1:  size = sizeof(Pds::CsPad::ConfigV1); break;
        case 2:  size = sizeof(Pds::CsPad::ConfigV2); break;
        case 3:  size = sizeof(Pds::CsPad::ConfigV3); break;
        case 4:  size = sizeof(Pds::CsPad::ConfigV4); break;
        case 5:  size = sizeof(Pds::CsPad::ConfigV5); break;
        default: size = 0; break;
        } break;
      case Pds::TypeId::Id_Cspad2x2Config:
        switch(_type.version()) {
        case 1:  size = sizeof(Pds::CsPad2x2::ConfigV1); break;
        case 2:  size = sizeof(Pds::CsPad2x2::ConfigV2); break;
        default: break;
        }
      default: break;
      }
      _payload = new char[size];
      memcpy(_payload,c._payload,size);
      _gainMap = c._gainMap;
    }
    ~ConfigCache() 
    { delete[] _payload; }
  public:
    bool data(TypeId        contains,
              const char*   payload,
              ndarray<const  int16_t,3>& da,
              ndarray<const uint16_t,1>& ta) const 
    {
      switch(_type.id()) {
      case Pds::TypeId::Id_CspadConfig:
        
#define CASE_DVSN(v) case v:                                            \
        { const Pds::CsPad::DataV##v& d =                               \
            *reinterpret_cast<const Pds::CsPad::DataV##v*>(payload);    \
          const Pds::CsPad::ElementV##v& e = d.quads(c,0);              \
          da = e.data(c);                                               \
          ta = e.sb_temp();                                             \
          return true;                                                  \
        } break;
#define CASE_CVSN(v) case v:                                            \
        { const Pds::CsPad::ConfigV##v& c =                             \
            *reinterpret_cast<Pds::CsPad::ConfigV##v*>(_payload);       \
          switch(contains.version()) {                                  \
            CASE_DVSN(1)                                                \
              CASE_DVSN(2)                                              \
              default: break;                                           \
          } } break;
      
      switch(_type.version()) {
        CASE_CVSN(2)
        CASE_CVSN(3)
        CASE_CVSN(4)
        CASE_CVSN(5)
        default: break;
      }
#undef CASE_CVSN
#undef CASE_DVSN
      break;

      case Pds::TypeId::Id_Cspad2x2Config:

#define CASE_DVSN(v) case v:                                            \
        { const Pds::CsPad2x2::ElementV##v& d =                         \
            *reinterpret_cast<const Pds::CsPad2x2::ElementV##v*>(payload); \
          da = d.data();                                                \
          ta = d.sb_temp();                                             \
          return true;                                                  \
        } break;
#define CASE_CVSN(v) case v:                                            \
        switch(contains.version()) {                                    \
          CASE_DVSN(1)                                                  \
            default: break;                                             \
          } break;
      
      switch(_type.version()) {
        CASE_CVSN(1)
        CASE_CVSN(2)

#undef CASE_CVSN
#undef CASE_DVSN

        default:
        break;
      } break;
      default: break;
      }

      printf("CspadMiniHandler::data cfgtype %08x datatype %08x\n",
             _type.value(), contains.value());

      da = ndarray<const  int16_t,3>();
      ta = ndarray<const uint16_t,1>();
      return false;
    }
  public:
    const ndarray<uint16_t,2>& gainMap() const { return _gainMap; }
  private:
    Pds::TypeId _type;
    char*       _payload;
    ndarray<uint16_t,2> _gainMap;
  };

  class Detector {
  public:
    Detector(const Src& src,
             const ConfigCache& c,
             double* f,    // offsets
             double* s,    // status
             double* g,    // gain
             double* rms,  // noise
             FILE* gm,   // geometry
             unsigned max_pixels,
	     const CspadTemp& therm) :
      _src   (src),
      _config(c),
      _therm (therm)
    {
      //  Determine layout : binning, origin
      double x,y;

      Ami::Cspad::QuadAlignment qalign = Ami::Cspad::QuadAlignment::qalign_def()[0];
      if (gm) {
        Ami::Cspad::QuadAlignment* nq = Ami::Cspad::QuadAlignment::load2x2(gm,true);
        qalign = *nq;
        delete nq;
      }
      else {
        //        for(unsigned i=0; i<8; i++)
        //qalign._twobyone[i]._rot = Ami::D90;
      }
      for(unsigned k=0; k<2; k++)
        printf("  2x1[%d]: %f %f %d\n", 
               k,
               qalign._twobyone[k]._pad.x,
               qalign._twobyone[k]._pad.y,
               qalign._twobyone[k]._rot);
      //
      //  Create a default layout
      //
      _pixels = 2048-256;
      _ppb = 1;
      { const double frame = double(_pixels)*pixel_size;
	x =  0.5*frame;
	y = -0.5*frame;
      }
      mini = new TwoByTwo(x,y,_ppb,&qalign._twobyone[0],_config.gainMap());

      //
      //  Test extremes and narrow the focus
      //
      unsigned xmin(_pixels), xmax(0), ymin(_pixels), ymax(0);
      for(unsigned i=0; i<2; i++) {
        unsigned x0,x1,y0,y1;
        mini->asic[i&1]->boundary(x0,x1,y0,y1);
        if (x0<xmin) xmin=x0;
        if (x1>xmax) xmax=x1;
        if (y0<ymin) ymin=y0;
        if (y1>ymax) ymax=y1;
      }

      delete mini;

      int idx = xmax-xmin+1;
      int idy = ymax-ymin+1;
      int pixels = ((idx>idy) ? idx : idy);
      const int bin0 = 4;
      _ppb = 1;

      x += pixel_size*double(bin0*int(_ppb) - int(xmin));
      y -= pixel_size*double(bin0*int(_ppb) - int(ymin));

      _pixels = pixels + 2*bin0*_ppb;

      mini = new TwoByTwo(x,y,_ppb,&qalign._twobyone[0],_config.gainMap(),
                          f,s,g,rms);
    }
    ~Detector() { delete mini; }

    void fill(Ami::DescImage&    image,
	      Ami::FeatureCache& cache) const
    {
      char buff[64];
      _cache = &cache;
      const char* detname = DetInfo::name(static_cast<const DetInfo&>(_src));
      mini->fill(image, (1<<2)-1);
      for(unsigned a=0; a<4; a++) {
        sprintf(buff,"%s:Temp[%d]",detname,a);
        _feature[a] = cache.add(buff);
      }
      if (Ami::EventHandler::post_diagnostics())
        for(unsigned a=0; a<4; a++) {
          sprintf(buff,"%s:CommonMode[%d]",detname,a);
          _feature[a+4] = cache.add(buff);
        }
      else 
        for(unsigned a=0; a<4; a++)
          _feature[a+4] = -1;
#ifdef POST_INTEGRAL
      sprintf(buff,"%s:Cspad::Sum",detname);
      _feature[8] = cache.add(buff);
#else
      _feature[8] = -1;
#endif
    }
    void fill(Ami::EntryImage& image,
	      const Xtc&       xtc) const
    {
      ndarray<const  int16_t,3> data;
      ndarray<const uint16_t,1> temp;
      if (!_config.data(xtc.contains,xtc.payload(),data,temp)) {
        printf("CspadMiniHandler::error extracting data\n");
        return;
      }

      for(int a=0; a<4; a++)
        _cache->cache(_feature[a],
                      _therm.getTemp(temp[a]));

      mini->fill(image,data,*_cache,_feature[4]);

#ifdef POST_INTEGRAL
      if (image.desc().options()&CspadCalib::option_post_integral()) {
        double s = 0;
        double p   = double(image.info(Ami::EntryImage::Pedestal));
        for(unsigned fn=0; fn<image.desc().nframes(); fn++) {
          int xlo(0), xhi(3000), ylo(0), yhi(3000);
          if (image.desc().xy_bounds(xlo, xhi, ylo, yhi, fn)) {
            for(int j=ylo; j<yhi; j++)
              for(int i=xlo; i<xhi; i++) {
                double v = double(image.content(i,j))-p;
                s += v;
              }
          }
        }
        _cache->cache(_feature[8],s);
      }
#endif
    }
    void fill(Ami::EntryImage& image, 
              double v0,double v1) const
    {
      mini->fill(image, (1<<2)-1, v0, v1);
    }
    void set_pedestals(double* f)
    {
      mini->set_pedestals(f);
    }
    void rename(const char* s) {
      char buff[64];
      for(unsigned a=0; a<4; a++) {
	sprintf(buff,"%s:Temp[%d]",s,a);
	_cache->rename(_feature[a],buff);
      }
      if (Ami::EventHandler::post_diagnostics())
        for(unsigned a=0; a<4; a++) {
          sprintf(buff,"%s:CommonMode[%d]",s,a);
          _cache->rename(_feature[4+a],buff);
        }
#ifdef POST_INTEGRAL
      sprintf(buff,"%s:Cspad:Sum",s);
      _cache->rename(_feature[8],buff);
#endif
    }
    unsigned ppb() const { return _ppb; }
    unsigned xpixels() { return _pixels; }
    unsigned ypixels() { return _pixels; }
    bool     used   () const {
      for(unsigned i=0; i<NumFeatures; i++)
	if (_cache->used(_feature[i])) return true;
      return false;
    }
  private:
    TwoByTwo* mini;
    const Src&  _src;
    ConfigCache _config;
    mutable Ami::FeatureCache* _cache;
    enum { NumFeatures=9 };
    mutable int _feature[NumFeatures];
    unsigned _ppb;
    unsigned _pixels;
    const CspadTemp& _therm;
  };

  class CspadMiniPFF : public Ami::PeakFinderFn {
  public:
    CspadMiniPFF(bool gain,
                 bool rms,
                 const Detector*  detector,
                 Ami::DescImage& image) :
      _detector(*detector),
      _nbinsx  (image.nbinsx()),
      _nbinsy  (image.nbinsy()),
      _values  (new Ami::EntryImage(image))
    {
      image.pedcalib (true);
      image.gaincalib(gain);
      image.rmscalib (rms );
    }
    virtual ~CspadMiniPFF()
    {
      delete _values;
    }
  public:
    void     setup(double v0,
                   double v1)
    {
      const unsigned no_threshold = -1;
      for(unsigned k=0; k<_nbinsy; k++)
        for(unsigned j=0; j<_nbinsx; j++)
          _values->content(no_threshold,j,k);

      _detector.fill(*_values,v0,v1);
    }
    unsigned value(unsigned j, unsigned k) const
    {
      return _values->content(j,k);
    }
    Ami::PeakFinderFn* clone() const { return new CspadMiniPFF(*this); }
  private:
    CspadMiniPFF(const CspadMiniPFF& o) :
      _detector(o._detector),
      _nbinsx  (o._nbinsx),
      _nbinsy  (o._nbinsy),
      _values  (new Ami::EntryImage(o._values->desc())) {}
  private:
    const Detector&  _detector;
    unsigned         _nbinsx;
    unsigned         _nbinsy;
    Ami::EntryImage* _values;
  };
};
};

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_CspadConfig);
  types.push_back(Pds::TypeId::Id_Cspad2x2Config);
  return types;
}

CspadMiniHandler::CspadMiniHandler(const Pds::DetInfo& info, FeatureCache& features, unsigned max_pixels) :
  EventHandler(info, Pds::TypeId::Id_Cspad2x2Element, config_type_list()),
  _entry(0),
  _detector(0),
  _cache(features),
  _max_pixels(max_pixels),
  _options   (0),
  _therm     (RDIV)
{
  unsigned s = sizeof(CspadMiniGeometry::off_no_ped)/sizeof(int16_t);
  for (unsigned i=0; i<s; i++) { 
    CspadMiniGeometry::off_no_ped[i] = (int16_t)Offset;
    CspadMiniGeometry::fgn_no_ped[i] = 1.;
  }
}

CspadMiniHandler::~CspadMiniHandler()
{
  if (_detector)
    delete _detector;

  Ami::PeakFinder::register_(info().phy(),NULL);
}

unsigned CspadMiniHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* CspadMiniHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

const Entry* CspadMiniHandler::hidden_entry(unsigned i) const { return 0; }

void CspadMiniHandler::rename(const char* s)
{
  if (_entry) {
    _entry->desc().name(s);
    _detector->rename(s);
  }
}

void CspadMiniHandler::reset() { _entry = 0; }

bool CspadMiniHandler::used() const
{
  return ((_entry && _entry->desc().used()) ||
	  (_detector && _detector->used()));
}

//
//  accomodate the different calib file formats
//
static ndarray<double,3> get_calib(FILE* f)
{
  ndarray<double,3> a;
  if (f) {
    a = make_ndarray<double>(2,Columns,Rows);
    
    size_t sz = 8 * 1024;
    char* linep = (char *)malloc(sz);
    Ami::Calib::get_line(&linep, &sz, f);
    double v;
    if (sscanf(linep,"%lf %lf %lf",&v,&v,&v)==2) {
      rewind(f);
      Ami::Calib::skip_header(f);
      double* p0 = &a(0,0,0);
      double* p1 = &a(1,0,0);
      while(p1 < a.end()) {
        fscanf(f,"%lf %lf",p0,p1);
        p0++;
        p1++;
      }
    }
    else {
      rewind(f);
      Ami::Calib::skip_header(f);
      for(double* p=a.begin(); p<a.end(); p++)
        fscanf(f,"%lf",p);
    }

    free(linep);
    fclose(f);
  }
  return a;
}

void CspadMiniHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
  //
  //  Load pedestals
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];

  const DetInfo& dInfo = static_cast<const Pds::DetInfo&>(info());
  ndarray<double,3> f = get_calib(Calib::fopen(dInfo, "ped", "pedestals"));
  ndarray<double,3> s = get_calib(Calib::fopen(dInfo, "sta", "pixel_status"));

  sprintf(oname1,"gain.%08x.dat",info().phy());
  sprintf(oname2,"/cds/group/pcds/pds/cspadcalib/gain.%08x.dat",info().phy());
  ndarray<double,3> g = get_calib(Calib::fopen_dual(oname1, oname2, "gain map"));

  ndarray<double,3> rms = get_calib(Calib::fopen(dInfo, "res", "pixel_rms"));

  bool offl_type=false;
  FILE* gm = Calib::fopen(dInfo,
                          "geo", "geometry", false,
                          &offl_type);
  if (gm && offl_type==false) {
    printf("Ignoring old-style online geometry constants\n");
    fclose(gm);
    gm = 0;
  }

  CspadMiniGeometry::ConfigCache cfg(type,payload);

  _create_entry( cfg,
                 f.size() ? f.begin():0,
                 s.size() ? s.begin():0,
                 g.size() ? g.begin():0,
                 rms.size() ? rms.begin():0,
                 gm,
                 _detector, _entry, _max_pixels);

  Ami::PeakFinder::register_(info().phy(),   
                             new CspadMiniGeometry::CspadMiniPFF(g.size(),rms.size(),
                                                                 _detector,_entry->desc()));
}

void CspadMiniHandler::_create_entry(const CspadMiniGeometry::ConfigCache& cfg,
                                     double* f,
                                     double* s,
                                     double* g,
                                     double* rms,
                                     FILE* gm,
                                     CspadMiniGeometry::Detector*& detector,
                                     EntryImage*& entry, 
                                     unsigned max_pixels) 
{
  if (detector)
    delete detector;

  detector = new CspadMiniGeometry::Detector(info(),cfg,f,s,g,rms,gm,max_pixels,_therm);
  if (gm) { fclose(gm); }

  const unsigned ppb = detector->ppb();
  const DetInfo& det = static_cast<const DetInfo&>(info());
  DescImage desc(det, ChannelID::name(det,0), "photons",
                 detector->xpixels()/ppb, detector->ypixels()/ppb, 
                 ppb, ppb, 
                 f!=0, g!=0, false);
  desc.set_scale(pixel_size*1e-3,pixel_size*1e-3);
    
  detector->fill(desc,_cache);

  entry = new EntryImage(desc);
  memset(entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));

#if 0
  if (f)
    entry->info(Offset*ppb*ppb,EntryImage::Pedestal);
  else
    entry->info(0,EntryImage::Pedestal);
#else
  entry->info(Offset*ppb*ppb,EntryImage::Pedestal);
#endif
    
  entry->info(0,EntryImage::Normalization);
  entry->invalid();
}

void CspadMiniHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void CspadMiniHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Xtc* xtc = reinterpret_cast<const Xtc*>(payload)-1;
  if (_entry) {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("CspadMiniHandler::event options %x -> %x\n", _options, o);
      _options = o;
    }

    if (_entry->desc().options() & CspadCalib::option_reload_pedestal()) {
      const DetInfo& dInfo = static_cast<const Pds::DetInfo&>(info());
      ndarray<double,3> f = get_calib(Calib::fopen(dInfo, "ped", "pedestals", true));
      if (f.size()) {
        _detector->set_pedestals(f.begin());
        _entry->desc().options( _entry->desc().options()&~CspadCalib::option_reload_pedestal() );
      }
    }

    _detector->fill(*_entry,*xtc);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void CspadMiniHandler::_damaged() 
{
  if (_entry) 
    _entry->invalid(); 
}

