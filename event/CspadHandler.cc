#include "CspadHandler.hh"
#include "CspadAlignment.hh"
#include "CspadAlignment_Commissioning.hh"

#include "ami/event/CspadCalib.hh"
#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/PeakFinder.hh"
#include "ami/data/PeakFinderFn.hh"

#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#define QUAD_CHECK
#define UNBINNED
#define DO_PED_CORR

using Ami::CspadCalib;

typedef Pds::CsPad::ElementV2 CspadElement;

static const unsigned Offset = 0x4000;
static const double  dOffset = double(Offset);
static const double pixel_size = 110;
static const float HI_GAIN_F = 1.;
static const float LO_GAIN_F = 7.;
static const double RDIV = 20000;

//
//  Much of the "template" code which follows is meant to 
//  factor the 90-degree rotations
//

static inline unsigned sum1(const int16_t*& data,
                            const int16_t*& off,
                            const int16_t* const*& psp,
                            const double fn,
                            const float*& gn)
{ 
  unsigned v;
  if (off==*psp) { psp++; v = Offset; }
  else { 
    //    double d = (*gn)*(double(*data + *off - Offset) - fn);
    double d = (*gn)*((double(*data + *off) - dOffset) - fn);
    //    double d = ((double(*data + *off) - dOffset) - fn);
    d += Offset; 
    v = unsigned(d+0.5);
  }
  data++;
  off++;
  gn++;
  return v;
}

static inline unsigned sum2(const int16_t*& data,
                            const int16_t*& off,
                            const int16_t* const*& psp,
                            const double fn,
                            const float*& gn)
{
  unsigned v;
  if (*psp && (*psp-off)<2) {
    off  += 2;
    data += 2;
    gn   += 2;
    v = 2*Offset;
    psp++;
    while(*psp && *psp<off) psp++;
  }
  else {
    double d; 
    d  = (*gn++)*(double(*data++ + *off++ - Offset)-fn);
    d += (*gn++)*(double(*data++ + *off++ - Offset)-fn);
    d += 2*Offset;
    v = unsigned(d+0.5);
  }
  return v; }

static inline unsigned sum4(const int16_t*& data,
                            const int16_t*& off,
                            const int16_t* const*& psp,
                            const double fn,
                            const float*& gn)
{
  unsigned v;
  if (*psp && (*psp-off)<4) {
    off  += 4;
    data += 4;
    gn   += 4;
    v = 4*Offset;
    psp++;
    while(*psp && *psp<off) psp++;
  }
  else {
    double d;
    d  = (*gn++)*(double(*data++ + *off++ - Offset)-fn);
    d += (*gn++)*(double(*data++ + *off++ - Offset)-fn);
    d += (*gn++)*(double(*data++ + *off++ - Offset)-fn);
    d += (*gn++)*(double(*data++ + *off++ - Offset)-fn); 
    d += 4*Offset; 
    v = unsigned(d+0.5); }
  return v; }

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

static inline unsigned thr2(double v0, double v1,
                            const int16_t*& off,
                            const int16_t* const*& psp,
                            const float*& rms)
{
  unsigned v;
  if (*psp && (*psp-off)<2) {
    off  += 2;
    v = no_threshold;
    psp++;
    while(*psp && *psp<off) psp++;
  }
  else {
    double d = 2*v0 + v1*(rms[0]+rms[1]);
    v = unsigned(d+0.5);
    off += 2;
  }
  rms += 2;
  return v; }

static inline unsigned thr4(double v0, double v1,
                            const int16_t*& off,
                            const int16_t* const*& psp,
                            const float*& rms)
{
  unsigned v;
  if (*psp && (*psp-off)<4) {
    off  += 4;
    v = no_threshold;
    psp++;
    while(*psp && *psp<off) psp++;
  }
  else {
    double d = 4*v0 + v1*(rms[0]+rms[1]+rms[2]+rms[3]);
    v = unsigned(d+0.5); 
    off += 4;
  }
  rms += 4;
  return v; }

#if 0
static double frameNoise(const int16_t*  data,
                         const int16_t*  off,
                         const int16_t* const* sta)
{
  double sum = 0;
  const unsigned ColBins = CsPad::ColumnsPerASIC;
  const unsigned RowBins = CsPad::MaxRowsPerASIC<<1;
  const int16_t* d(data);
  const int16_t* o(off );
  for(unsigned i=0; i<ColBins; i++) {
    for(unsigned j=0; j<RowBins; j++, d++, o++) {
      int v = *d + *o - Offset;
      sum += double(v);
    }
  }
  return sum/double(ColBins*RowBins);
}
#else
static double frameNoise(const int16_t*  data,
                         const int16_t*  off,
                         const int16_t* const* sta,
                         bool noped = false)
{
  const unsigned ColBins = CsPad::ColumnsPerASIC;
  const unsigned RowBins = CsPad::MaxRowsPerASIC<<1;
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
      for(unsigned j=0; j<RowBins; j++, d++, o++) {
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
  { const int fnPeakBins = 3;
    const int fnPixelRange = fnPixelBins-fnPeakBins-1;
    const unsigned fnPedestalThreshold = 100;
    
    unsigned i=fnPeakBins+1;
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
    }
    else
      //      printf("frameNoise : peak not found\n");
      ;
  }

  return v;
}
#endif

static double unbondedNoise(const int16_t*  data,
                            const int16_t*  off)
{
#define CORR(i) (data[i]+off[i])
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
namespace CspadGeometry {

  //
  //  When filling the image, compensate data which
  //    only partially fills a pixel (at the edges)
  //
#define FRAME_BOUNDS                                            \
  const unsigned ColLen   = (  CsPad::ColumnsPerASIC+ppb-1)/ppb-1;	\
    const unsigned RowLen = (2*CsPad::MaxRowsPerASIC+ppb-1)/ppb-1;	\
    unsigned x0 = CALC_X(column,0,0);                           \
    unsigned x1 = CALC_X(column,ColLen,RowLen);                 \
    unsigned y0 = CALC_Y(row,0,0);                              \
    unsigned y1 = CALC_Y(row,ColLen,RowLen);                    \
    if (x0 > x1) { unsigned t=x0; x0=x1; x1=t; }                \
    if (y0 > y1) { unsigned t=y0; y0=y1; y1=t; }      


#define BIN_ITER4(F4) {                                         \
    const unsigned ColBins = CsPad::ColumnsPerASIC>>2;          \
    const unsigned RowBins = CsPad::MaxRowsPerASIC>>1;          \
    /*  zero the target region  */                              \
    for(unsigned i=0; i<=ColBins; i++) {                        \
      for(unsigned j=0; j<=RowBins; j++) {                      \
        const unsigned x = CALC_X(column,i,j);                  \
        const unsigned y = CALC_Y(row   ,i,j);                  \
        image.content(0,x,y);                                   \
      }                                                         \
    }                                                           \
    /*  fill the target region  */                              \
    for(unsigned i=0; i<ColBins; i++) {                         \
      for(unsigned k=0; k<4; k++) {                             \
        unsigned j=0; double fn=fn1;                            \
        while(j < RowBins>>1) {                                 \
          const unsigned x = CALC_X(column,i,j);                \
          const unsigned y = CALC_Y(row   ,i,j);                \
          image.addcontent(F4,x,y);                             \
          j++;                                                  \
        }                                                       \
        fn=fn2;                                                 \
        while(j < RowBins) {                                    \
          const unsigned x = CALC_X(column,i,j);                \
          const unsigned y = CALC_Y(row   ,i,j);                \
          image.addcontent(F4,x,y);                             \
          j++;                                                  \
        }                                                       \
      }                                                         \
    }                                                           \
    unsigned j=0; double fn=fn1;                                \
    while(j < RowBins>>1) {                                     \
      const unsigned x = CALC_X(column,ColBins,j);              \
      const unsigned y = CALC_Y(row   ,ColBins,j);              \
      image.addcontent(4*F4,x,y);                               \
      j++;                                                      \
    }                                                           \
    fn=fn2;                                                     \
    while(j < RowBins) {                                        \
      const unsigned x = CALC_X(column,ColBins,j);              \
      const unsigned y = CALC_Y(row   ,ColBins,j);              \
      image.addcontent(4*F4,x,y);                               \
      j++;                                                      \
    }                                                           \
  }

#define BIN_ITER2(F2) {                                         \
    const unsigned ColBins = CsPad::ColumnsPerASIC>>1;          \
    const unsigned RowBins = CsPad::MaxRowsPerASIC;             \
    /*  zero the target region  */                              \
    for(unsigned i=0; i<=ColBins; i++) {                        \
      for(unsigned j=0; j<=RowBins; j++) {                      \
        const unsigned x = CALC_X(column,i,j);                  \
        const unsigned y = CALC_Y(row   ,i,j);                  \
        image.content(0,x,y);                                   \
      }                                                         \
    }                                                           \
    /*  fill the target region  */                              \
    for(unsigned i=0; i<ColBins; i++) {                         \
      for(unsigned k=0; k<2; k++) {                             \
        unsigned j=0; double fn=fn1;                            \
        while(j < RowBins>>1) {                                 \
          const unsigned x = CALC_X(column,i,j);                \
          const unsigned y = CALC_Y(row   ,i,j);                \
          image.addcontent(F2,x,y);                             \
          j++;                                                  \
        }                                                       \
        fn=fn2;                                                 \
        while(j < RowBins) {                                    \
          const unsigned x = CALC_X(column,i,j);                \
          const unsigned y = CALC_Y(row   ,i,j);                \
          image.addcontent(F2,x,y);                             \
          j++;                                                  \
        }                                                       \
      }                                                         \
    }                                                           \
    unsigned j=0; double fn=fn1;                                \
    while(j < RowBins>>1) {                                     \
      const unsigned x = CALC_X(column,ColBins,j);              \
      const unsigned y = CALC_Y(row   ,ColBins,j);              \
      image.addcontent(2*F2,x,y);                               \
      j++;                                                      \
    }                                                           \
    fn=fn2;                                                     \
    while(j < RowBins) {                                        \
      const unsigned x = CALC_X(column,ColBins,j);              \
      const unsigned y = CALC_Y(row   ,ColBins,j);              \
      image.addcontent(2*F2,x,y);                               \
      j++;                                                      \
    }                                                           \
  }

#define BIN_ITER1(F1) {                                 \
    const unsigned ColBins = CsPad::ColumnsPerASIC;     \
    const unsigned RowBins = CsPad::MaxRowsPerASIC;     \
    /*  fill the target region  */                      \
    for(unsigned i=0; i<ColBins; i++) {                 \
      unsigned j=0; double fn=fn1;                      \
      while(j < RowBins) {                              \
        const unsigned x = CALC_X(column,i,j);          \
        const unsigned y = CALC_Y(row   ,i,j);          \
        image.content(F1,x,y);                          \
        j++;                                            \
      }                                                 \
      fn=fn2;                                           \
      while(j < RowBins*2) {                            \
        const unsigned x = CALC_X(column,i,j);          \
        const unsigned y = CALC_Y(row   ,i,j);          \
        image.content(F1,x,y);                          \
        j++;                                            \
      }                                                 \
    }                                                   \
  }

  //
  //  This class locates the ASIC data to the binned image grid
  //
  class Asic {
  public:
    Asic(double x, double y, unsigned ppbin) :
      column(unsigned( x/pixel_size)/ppbin),
      row   (unsigned(-y/pixel_size)/ppbin),
      ppb(ppbin) {}
    virtual ~Asic() {}
  public:
    virtual void fill(Ami::DescImage& image) const = 0;
    virtual void fill(Ami::EntryImage&   image,
                      const int16_t*     data,
                      Ami::FeatureCache& cache,
                      unsigned           index) const = 0;
    virtual void fill(Ami::EntryImage& image,
                      double v0, double v1) const = 0;
    virtual void set_pedestals(FILE*) {}
  public:
    virtual void boundary(unsigned& x0, unsigned& x1, 
        unsigned& y0, unsigned& y1) const = 0;
  protected:
    unsigned column, row;
    unsigned ppb;
    int16_t*  _sta[CsPad::MaxRowsPerASIC*CsPad::ColumnsPerASIC*2];
  };

  class AsicP : public Asic {
  public:
    AsicP(double x, double y, unsigned ppbin, FILE* ped, FILE* status, FILE* gain, FILE* rms,
          const ndarray<const uint16_t,2>& gmap, unsigned imap) :
      Asic(x,y,ppbin)
    { // load offset-pedestal 
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      memset(linep, 0, sz);
      char* pEnd = linep;

      if (ped) {
        int16_t* off = _off;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          Ami::Calib::get_line(&linep, &sz, ped);
          *off++ = Offset - int16_t(strtod(linep,&pEnd));
          for (unsigned row=1; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *off++ = Offset - int16_t(strtod(pEnd, &pEnd));
        }
      }
      else {
        int16_t* off = _off;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++)
          for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *off++ = Offset;
      }

      if (status) {
        int16_t*  off = _off;
        int16_t** sta = _sta;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          Ami::Calib::get_line(&linep, &sz, status);
          if (strtoul(linep,&pEnd,0)) *sta++ = off;
          off++;
          for (unsigned row=1; row < 2*Pds::CsPad::MaxRowsPerASIC; row++, off++)
            if (strtoul(pEnd,&pEnd,0)) *sta++ = off;
        }
        *sta = 0;
      }
      else
        _sta[0] = 0;

      if (gain) {
        float* gn = _gn;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          Ami::Calib::get_line(&linep, &sz, gain);
          *gn++ = strtod(linep,&pEnd);
          for (unsigned row=1; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *gn++ = strtod(pEnd,&pEnd);
        }
      }
      else {
        float* gn = _gn;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *gn++ = 1.;
        }
      }

      { float* gn = _gn;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          for (unsigned row=0; row < Pds::CsPad::MaxRowsPerASIC; row++)
            *gn++ *= ((gmap[col][row]>>imap)&1) ? HI_GAIN_F:LO_GAIN_F;
          for (unsigned row=0; row < Pds::CsPad::MaxRowsPerASIC; row++)
            *gn++ *= ((gmap[col][row]>>imap)&2) ? HI_GAIN_F:LO_GAIN_F;
        }
      }

      if (rms) {
        float* r = _rms;
        float* g = _gn;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          Ami::Calib::get_line(&linep, &sz, rms);
          *r++ = strtod(linep,&pEnd);
          for (unsigned row=1; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *r++ = strtod(pEnd,&pEnd)*(*g++);
        }
      }
      else {
        float* r = _rms;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *r++ = 0;
        }
      }
      
      if (linep) {
        free(linep);
      }
    }
    void set_pedestals(FILE* ped) 
    {
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      memset(linep, 0, sz);
      char* pEnd = linep;

      if (ped) {
        int16_t* off = _off;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++) {
          Ami::Calib::get_line(&linep, &sz, ped);
          *off++ = Offset - int16_t(strtod(linep,&pEnd));
          for (unsigned row=1; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *off++ = Offset - int16_t(strtod(pEnd, &pEnd));
        }
      }
      else {
        int16_t* off = _off;
        for(unsigned col=0; col<CsPad::ColumnsPerASIC; col++)
          for (unsigned row=0; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
            *off++ = Offset;
      }

      if (linep)
        free(linep);
    }
    void kill_off() { memset(_off,0,sizeof(_off)); }
  protected:
    int16_t  _off[CsPad::MaxRowsPerASIC*CsPad::ColumnsPerASIC*2];
    int16_t* _sta[CsPad::MaxRowsPerASIC*CsPad::ColumnsPerASIC*2+1];
    float    _gn [CsPad::MaxRowsPerASIC*CsPad::ColumnsPerASIC*2];
    float    _rms[CsPad::MaxRowsPerASIC*CsPad::ColumnsPerASIC*2];
  };

  static int16_t  off_no_ped[CsPad::MaxRowsPerASIC*CsPad::ColumnsPerASIC*2];
  static float    fgn_no_ped[CsPad::MaxRowsPerASIC*CsPad::ColumnsPerASIC*2];

#define AsicTemplate(classname,bi,ti,PPB,rot)                           \
  class classname : public AsicP {                                      \
  public:                                                               \
    classname(double x, double y,                                       \
              FILE* p, FILE* s, FILE* g, FILE* rms,                     \
              const ndarray<const uint16_t,2>& gmap, unsigned imap)     \
      : AsicP(x,y,PPB,p,s,g,rms,gmap,imap) {}                           \
    void boundary(unsigned& dx0, unsigned& dx1,                         \
                  unsigned& dy0, unsigned& dy1) const {                 \
      FRAME_BOUNDS;                                                     \
      printf("Boundary [%u[%u,%u],%u[%u,%u]]\n",                        \
             column,x0,x1,row,y0,y1);                                   \
      dx0=x0; dx1=x1; dy0=y0; dy1=y1; }                                 \
    void fill(Ami::DescImage& image) const {                            \
      FRAME_BOUNDS;                                                     \
      image.add_frame(x0,y0,x1-x0+1,y1-y0+1,rot);                       \
    }                                                                   \
    void fill(Ami::EntryImage& image,                                   \
              double v0, double v1) const                               \
    {                                                                   \
      bool lsuppress  = image.desc().options()&CspadCalib::option_suppress_bad_pixels(); \
      int16_t* zero = 0;                                               \
      const int16_t* off = _off;                                       \
      const int16_t* const * sta = lsuppress ? _sta : &zero;           \
      const float* rms = _rms;                                          \
      double fn1=0,fn2=0;                                               \
      ti;                                                               \
    }                                                                   \
    void fill(Ami::EntryImage&   image,                                 \
              const int16_t*     data,                                  \
              Ami::FeatureCache& cache,                                 \
              unsigned           ifn) const {                           \
      bool lsuppress  = image.desc().options()&CspadCalib::option_suppress_bad_pixels(); \
      bool lcorrectfn = image.desc().options()&CspadCalib::option_correct_common_mode(); \
      bool lcorrectun = image.desc().options()&CspadCalib::option_correct_unbonded(); \
      bool lcorrectgn = image.desc().options()&CspadCalib::option_correct_gain(); \
      bool lnopedestal= image.desc().options()&CspadCalib::option_no_pedestal(); \
      int16_t* zero = 0;                                                \
      const int16_t* off = lnopedestal ? off_no_ped : _off;             \
      const int16_t* const * sta = lsuppress ? _sta : &zero;            \
      const float* gn = (lnopedestal || !lcorrectgn) ? fgn_no_ped :_gn; \
      double fn1=0,fn2=0;                                               \
      if (lcorrectfn) { fn1 = fn2 = frameNoise(data,off,sta); }         \
      else if(lcorrectun) {                                             \
        fn1 = unbondedNoise(data,off);                                  \
        fn2 = unbondedNoise(data+CsPad::MaxRowsPerASIC,                 \
                            off +CsPad::MaxRowsPerASIC);                \
      }                                                                 \
      if (Ami::EventHandler::post_diagnostics()) {                      \
        cache.cache(ifn,fn1);                                           \
        cache.cache(ifn+1,fn2);                                         \
      }                                                                 \
      bi;                                                               \
    }                                                                   \
  }

#define B1 { BIN_ITER1(sum1(data,off,sta,fn,gn)) }
#define B2 { BIN_ITER2(sum2(data,off,sta,fn,gn)) }
#define B4 { BIN_ITER4(sum4(data,off,sta,fn,gn)) }
#define T1 { BIN_ITER1(thr1(v0,v1,off,sta,rms)) }
#define T2 { BIN_ITER2(thr2(v0,v1,off,sta,rms)) }
#define T4 { BIN_ITER4(thr4(v0,v1,off,sta,rms)) }

#define CALC_X(a,b,c) (a+b)         
#define CALC_Y(a,b,c) (a-c)          
  AsicTemplate(  AsicD0B1P, B1, T1, 1, D0);
  AsicTemplate(  AsicD0B2P, B2, T2, 2, D0);
  AsicTemplate(  AsicD0B4P, B4, T4, 4, D0);
#undef CALC_X
#undef CALC_Y
#define CALC_X(a,b,c) (a+c)         
#define CALC_Y(a,b,c) (a+b)          
  AsicTemplate( AsicD90B1P, B1, T1, 1, D90);
  AsicTemplate( AsicD90B2P, B2, T2, 2, D90);
  AsicTemplate( AsicD90B4P, B4, T4, 4, D90);
#undef CALC_X
#undef CALC_Y
#define CALC_X(a,b,c) (a-b)         
#define CALC_Y(a,b,c) (a+c)          
  AsicTemplate(AsicD180B1P, B1, T1, 1, D180);
  AsicTemplate(AsicD180B2P, B2, T2, 2, D180);
  AsicTemplate(AsicD180B4P, B4, T4, 4, D180);
#undef CALC_X
#undef CALC_Y
#define CALC_X(a,b,c) (a-c)         
#define CALC_Y(a,b,c) (a-b)          
  AsicTemplate(AsicD270B1P, B1, T1, 1, D270);
  AsicTemplate(AsicD270B2P, B2, T2, 2, D270);
  AsicTemplate(AsicD270B4P, B4, T4, 4, D270);
#undef CALC_X
#undef CALC_Y

#undef B1
#undef B2
#undef B4
#undef T1
#undef T2
#undef T4
#undef AsicTemplate

  class TwoByTwo {
  public:
    TwoByTwo(double x, double y, unsigned ppb, 
             const Ami::Cspad::TwoByOneAlignment* a,
             FILE* f, FILE* s, FILE* g, FILE* rms,
             const ndarray<const uint16_t,2>& gmap, unsigned imap) 
    {
      for(unsigned i=0; i<2; i++, imap+=2) {
        double tx = a[i]._pad.x + x;
        double ty = a[i]._pad.y + y;
        //
        //  We may acquire pedestals between configurations
        //
        switch(a[i]._rot) {
        case D0: 
          switch(ppb) {
          case 1:       asic[i] = new  AsicD0B1P(tx,ty,f,s,g,rms,gmap,imap); break;
          case 2:       asic[i] = new  AsicD0B2P(tx,ty,f,s,g,rms,gmap,imap); break;
          default:      asic[i] = new  AsicD0B4P(tx,ty,f,s,g,rms,gmap,imap); break;
          } break;
        case D90:
          switch(ppb) {
          case 1:       asic[i] = new  AsicD90B1P(tx,ty,f,s,g,rms,gmap,imap); break;
          case 2:       asic[i] = new  AsicD90B2P(tx,ty,f,s,g,rms,gmap,imap); break;
          default:      asic[i] = new  AsicD90B4P(tx,ty,f,s,g,rms,gmap,imap); break;
          } break;
        case D180:
          switch(ppb) {
          case 1:       asic[i] = new  AsicD180B1P(tx,ty,f,s,g,rms,gmap,imap); break;
          case 2:       asic[i] = new  AsicD180B2P(tx,ty,f,s,g,rms,gmap,imap); break;
          default:      asic[i] = new  AsicD180B4P(tx,ty,f,s,g,rms,gmap,imap); break;
          } break;
        case D270:
          switch(ppb) {
          case 1:       asic[i] = new  AsicD270B1P(tx,ty,f,s,g,rms,gmap,imap); break;
          case 2:       asic[i] = new  AsicD270B2P(tx,ty,f,s,g,rms,gmap,imap); break;
          default:      asic[i] = new  AsicD270B4P(tx,ty,f,s,g,rms,gmap,imap); break;
          } break;
        default:
          break;
        }
      }
    }
    ~TwoByTwo() {  for(unsigned i=0; i<2; i++) delete asic[i]; }
    void fill(Ami::DescImage& image,
              unsigned        mask) const
    {
      if (mask&1) asic[0]->fill(image);
      if (mask&2) asic[1]->fill(image);
    }
    void fill(Ami::EntryImage&           image,
              const int16_t*             sector,
              unsigned                   sector_id,
              Ami::FeatureCache&         cache,
              unsigned                   index) const
    {
      asic[sector_id&1]->fill(image,sector,cache,index);
    }
    void fill(Ami::EntryImage& image,
              unsigned mask,
              double v0, double v1) const
    {
      if (mask&1) asic[0]->fill(image,v0,v1);
      if (mask&2) asic[1]->fill(image,v0,v1);
    }
    void set_pedestals(FILE* f)
    {
      for(unsigned ie=0; ie<2; ie++)
        asic[ie]->set_pedestals(f);
    }
  public:
    Asic* asic[2];
  };

  class Quad {
  public:
    Quad(double x, double y, unsigned ppb, 
         const Ami::Cspad::QuadAlignment& align,
         const ndarray<const uint16_t,2>& gmap,
         FILE* pedFile=0, FILE* staFile=0, FILE* gainFile=0, FILE* rmsFile=0)
    {
      for(unsigned i=0; i<4; i++) {
        element[i] = new TwoByTwo( x, y, ppb, &align._twobyone[i*2], 
                                   pedFile, staFile, gainFile, rmsFile, gmap, i*4 );
      }
    }
    ~Quad() { for(unsigned i=0; i<4; i++) delete element[i]; }
  public:
    void fill(Ami::DescImage&    image,
              unsigned           mask) const
    {
      for(unsigned i=0; i<4; i++, mask>>=2)
        if (mask&3)
          element[i]->fill(image, mask&3);
    }
    void fill(Ami::EntryImage&                image,
              const ndarray<const int16_t,3>& a,
              unsigned                        mask,
              Ami::FeatureCache&              cache,
              unsigned                        index) const
    {
      for(unsigned id=0,j=0; mask!=0; id++)
        if (mask&(1<<id)) {
	  mask ^= (1<<id);
          element[id>>1]->fill(image,&a[j][0][0],id,cache,index+id*2);
          j++;
        }
    }      
    void fill(Ami::EntryImage& image,
              unsigned mask,
              double v0, double v1) const
    {
      for(unsigned ie=0; ie<4; ie++, mask>>=2)
        if (mask&3)
          element[ie]->fill(image,mask&3,v0,v1);
    }
    void set_pedestals(FILE* f)
    {
      for(unsigned ie=0; ie<4; ie++)
        element[ie]->set_pedestals(f);
    }
  public:
    TwoByTwo* element[4];
  };

  class ConfigCache {
  public:
    ConfigCache(Pds::TypeId type, const void* payload) : 
      _type(type)
    {
#define CASE_VSN(v) case v:                                             \
      { const Pds::CsPad::ConfigV##v& c =                               \
          *reinterpret_cast<const Pds::CsPad::ConfigV##v*>(payload);    \
        size = sizeof(c);                                               \
        _quadMask   = c.quadMask();                                     \
        for(unsigned i=0; i<4; i++)                                     \
          _roiMask[i] = c.roiMask(i);                                   \
        for(unsigned i=0; i<4; i++)                                     \
          _gainMap[i] = c.quads(i).gm().gainMap().copy();               \
        break; }

      for(unsigned i=0; i<4; i++)
        _gainMap[i] = make_ndarray<uint16_t>(CsPad::ColumnsPerASIC,
                                             CsPad::MaxRowsPerASIC);

      unsigned size;
      switch(type.version()) {
      case 1:  
        { const Pds::CsPad::ConfigV1& c = 
            *reinterpret_cast<const Pds::CsPad::ConfigV1*>(payload); 
          size = sizeof(c);
          _quadMask   = c.quadMask();
          for(unsigned i=0; i<4; i++)
            _roiMask[i] = (_quadMask&(1<<i)) ? 0xff : 0;
          for(unsigned i=0; i<4; i++)                                     
            _gainMap[i] = c.quads(i).gm().gainMap().copy();               
          break; }
        CASE_VSN(2)
        CASE_VSN(3)
        CASE_VSN(4)
        CASE_VSN(5)
      default:
	printf("CspadHandler: unrecognized configuration version\n");
	size = 0;
	_quadMask = 0;
	memset(_roiMask,0,4*sizeof(unsigned));
      }
      _payload = new char[size];
      memcpy(_payload,payload,size);

#undef CASE_VSN
    }
    ConfigCache(const ConfigCache& c) : _type(c._type)
    { 
      unsigned size;
      switch(_type.version()) {
      case 1:  size = sizeof(Pds::CsPad::ConfigV1); break;
      case 2:  size = sizeof(Pds::CsPad::ConfigV2); break;
      case 3:  size = sizeof(Pds::CsPad::ConfigV3); break;
      case 4:  size = sizeof(Pds::CsPad::ConfigV4); break;
      case 5:  size = sizeof(Pds::CsPad::ConfigV5); break;
      default: size = 0; break;
      }
      _payload = new char[size];
      memcpy(_payload,c._payload,size);
      _quadMask = c._quadMask;
      for(unsigned i=0; i<4; i++)
        _roiMask[i] = c._roiMask[i];
      for(unsigned i=0; i<4; i++)
        _gainMap[i] = c._gainMap[i];
    }
    ~ConfigCache() 
    { delete[] _payload; }
  public:
    bool validate(TypeId      contains,
                  const char* payload) const
    {
#define CASE_DVSN(v) case v:                                            \
      { const Pds::CsPad::DataV##v& d =                                 \
          *reinterpret_cast<const Pds::CsPad::DataV##v*>(payload);      \
        unsigned mask(0);                                               \
        for(unsigned i=0; i<c.numQuads(); i++) {                        \
          const Pds::CsPad::ElementV##v& e = d.quads(c,i);              \
          if (mask&(1<<e.quad())) {                                     \
            printf("CsPad: Found duplicate quad %d.  Invalidating.\n",  \
                   e.quad());                                           \
            return false;                                               \
          }                                                             \
          mask |= (1<<e.quad());                                        \
        } } break;
#define CASE_CVSN(v) case v:                                            \
      { const Pds::CsPad::ConfigV##v& c =                               \
          *reinterpret_cast<Pds::CsPad::ConfigV##v*>(_payload);         \
        switch(contains.version()) {                                    \
          CASE_DVSN(1)                                                  \
          CASE_DVSN(2)                                                  \
            default: break;                                             \
        } } break;
      
      switch(_type.version()) {
      case 1:
      { const Pds::CsPad::ConfigV1& c =
          *reinterpret_cast<Pds::CsPad::ConfigV1*>(_payload);
        switch(contains.version()) {
          CASE_DVSN(1)
            default: break;
        } } break;
        CASE_CVSN(2)
        CASE_CVSN(3)
        CASE_CVSN(4)
        CASE_CVSN(5)
      default:
        break;
      }
#undef CASE_CVSN
#undef CASE_DVSN
      return true;
    }
    
    bool data(TypeId        contains,
              const char*   payload,
              unsigned      quad, 
              unsigned&     mask,
              ndarray<const int16_t ,3>& da,
              ndarray<const uint16_t,1>& ta) const 
    {
#define CASE_DVSN(v) case v:                                            \
      { const Pds::CsPad::DataV##v& d =                                 \
          *reinterpret_cast<const Pds::CsPad::DataV##v*>(payload);      \
        for(unsigned i=0; i<c.numQuads(); i++) {                        \
          const Pds::CsPad::ElementV##v& e = d.quads(c,i);              \
          if (e.quad()==quad) {                                         \
            mask = e.sectionMask(c);                                    \
            da = e.data(c);                                             \
            ta = e.sb_temp();                                           \
            return true;                                                \
          } } } break;
#define CASE_CVSN(v) case v:                                            \
      { const Pds::CsPad::ConfigV##v& c =                               \
          *reinterpret_cast<Pds::CsPad::ConfigV##v*>(_payload);         \
        switch(contains.version()) {                                    \
          CASE_DVSN(1)                                                  \
          CASE_DVSN(2)                                                  \
            default: break;                                             \
        } } break;
      
      switch(_type.version()) {
      case 1:
      { const Pds::CsPad::ConfigV1& c =
          *reinterpret_cast<Pds::CsPad::ConfigV1*>(_payload);
        switch(contains.version()) {
          CASE_DVSN(1)
            default: break;
        } } break;
        CASE_CVSN(2)
        CASE_CVSN(3)
        CASE_CVSN(4)
        CASE_CVSN(5)
      default:
        break;
      }
#undef CASE_CVSN
#undef CASE_DVSN
      mask = 0;
      da = ndarray<const  int16_t,3>();
      ta = ndarray<const uint16_t,1>();
      return false;
    }

  public:
    unsigned quadMask()           const { return _quadMask; }
    unsigned roiMask (unsigned i) const { return _roiMask[i]; }
    const ndarray<uint16_t,2>& gainMap(unsigned i) const { return _gainMap[i]; }
  private:
    Pds::TypeId _type;
    char*       _payload;
    unsigned    _quadMask;
    unsigned    _roiMask[4];
    ndarray<uint16_t,2> _gainMap[4];
  };

  class Detector {
  public:
    Detector(Ami::EventHandlerF& hdl,
             const Src& src,
             const ConfigCache& c,
             FILE* f,    // offsets
             FILE* s,    // status
             FILE* g,    // gain
             FILE* rms,  // noise
             unsigned max_pixels,
             bool     full_resolution,
	     const CspadTemp& therm) :
      _hdl   (hdl),
      _src   (src),
      _config(c),
      _therm (therm)
    {
      unsigned smask = 
        (_config.roiMask(0)<< 0) |
        (_config.roiMask(1)<< 8) |
        (_config.roiMask(2)<<16) |
        (_config.roiMask(3)<<24);

      //  Determine layout : binning, origin
      double x,y;

      const Ami::Cspad::QuadAlignment* qalign = qalign_def;

      bool offl_type=false;
      FILE *gm = Calib::fopen(static_cast<const Pds::DetInfo&>(src),
                              "geo", "geometry", false,
                              &offl_type);
      if (gm) {
        if (offl_type==false) {
          fclose(gm);
          gm = 0;
        }
        else {
          qalign = Ami::Cspad::QuadAlignment::load(gm,offl_type);
          { for(unsigned j=0; j<4; j++) {
              printf("Quad %d:\n", j);
              for(unsigned k=0; k<8; k++)
                printf("  2x1[%d]: %f %f %d\n", 
                       k,
                       qalign[j]._twobyone[k]._pad.x,
                       qalign[j]._twobyone[k]._pad.y,
                       qalign[j]._twobyone[k]._rot);
            }
          }
        }
      }

      //
      //  Create a default layout
      //
      _pixels = 2048-256;
      _ppb = 4;
      { 
        const double frame = double(_pixels)*pixel_size;
        x =   1.0*frame;
        y =  -1.0*frame;
      }
      for(unsigned j=0; j<4; j++)
        quad[j] = new Quad(x,y,_ppb,qalign[j],_config.gainMap(j));

      //
      //  Test extremes and narrow the focus
      //
      unsigned xmin(INT_MAX), xmax(0), ymin(INT_MAX), ymax(0);
      for(unsigned i=0; i<32; i++) {
        if (smask&(1<<i)) {
          unsigned x0,x1,y0,y1;
          quad[i>>3]->element[(i>>1)&3]->asic[i&1]->boundary(x0,x1,y0,y1);
          if (x0<xmin) xmin=x0;
          if (x1>xmax) xmax=x1;
          if (y0<ymin) ymin=y0;
          if (y1>ymax) ymax=y1;
        }
      }

      printf("Cspad geometry range (%f,%f) [%u,%u] [%u,%u]\n",
             x,y,xmin,xmax,ymin,ymax);

      for(int i=0; i<4; i++)
        delete quad[i];

      int idx = xmax-xmin+1;
      int idy = ymax-ymin+1;
      int pixels = ((idx>idy) ? idx : idy);
      const int bin0 = 4;
      _ppb = 1;

      if (!full_resolution) {
        while((pixels*4/_ppb+2*bin0) > max_pixels)
          _ppb<<=1;
      }

      x += pixel_size*double(bin0*int(_ppb) - xmin*4);
      y -= pixel_size*double(bin0*int(_ppb) - ymin*4);

      _pixels = pixels*4 + 2*bin0*_ppb;

      for(unsigned j=0; j<4; j++)
        quad[j] = new Quad(x,y,_ppb,qalign[j],_config.gainMap(j),f,s,g,rms);

      if (gm && offl_type)
        delete[] qalign;
    }
    ~Detector() { for(unsigned i=0; i<4; i++) delete quad[i]; }

    void fill(Ami::DescImage&    image) const
    {
      //
      //  The configuration should tell us how many elements to view
      //
      _hdl.reset();
      char buff[64];
      unsigned qmask = _config.quadMask();
      const char* detname = DetInfo::name(static_cast<const DetInfo&>(_src).detector());
      for(unsigned i=0; i<4; i++) {
        if (qmask & (1<<i)) {
          quad[i]->fill(image, _config.roiMask(i));
          for(unsigned a=0; a<4; a++) {
            sprintf(buff,"%s:Cspad:Quad[%d]:Temp[%d]",detname,i,a);
            _feature[20*i+a] = _hdl._add_to_cache(buff);
          }
          if (Ami::EventHandler::post_diagnostics()) {
            for(unsigned a=0; a<16; a++) {
              sprintf(buff,"%s:Cspad:Quad[%d]:CommonMode[%d]",detname,i,a);
              _feature[20*i+a+4] = _hdl._add_to_cache(buff);
            }
          }
	  else 
            for(unsigned a=0; a<16; a++)
	      _feature[20*i+a+4] = -1;
        }
	else 
	  for(unsigned a=0; a<20; a++)
	    _feature[20*i+a] = -1;
      }      

      sprintf(buff,"%s:Cspad:Sum",detname);
      _feature[80] = _hdl._add_to_cache(buff);
    }
    bool fill(Ami::EntryImage& image,
              Ami::FeatureCache& cache,
              TypeId           contains,
              const char*      payload,
              size_t           sizeofPayload) const
    {
#ifdef QUAD_CHECK
      //
      //  First, check for duplicate quads
      //
      if (!_config.validate(contains, payload)) {
        return false;
      }
#endif

      unsigned nframes[5];
      nframes[0] = 0;
      for(unsigned j=0; j<4; j++) {
        unsigned m;
        m = _config.roiMask(j);
        unsigned n = nframes[j];
        while(m) {
          m = m&(m-1);
          n++;
        }
        nframes[j+1] = n;
      }

      int q;
      Quad* const* quad = this->quad;
      double sum = 0;
      Ami::FeatureCache* pcache = &cache;
#ifdef _OPENMP
#pragma omp parallel shared(quad,pcache) private(q) num_threads(4)
      {
#pragma omp for schedule(dynamic,1)
#endif
        for(q=0; q<4; q++) {
          unsigned mask;
          ndarray<const  int16_t,3> data;
          ndarray<const uint16_t,1> temp;
          if (!_config.data(contains,payload,q,mask,data,temp)) continue;

          quad[q]->fill(image,data,mask,*pcache,_feature[20*q+4]);

          for(int a=0; a<4; a++)
            cache.cache(_feature[20*q+a],
                        _therm.getTemp(temp[a]));

          //  Calculate integral
          if (image.desc().options()&CspadCalib::option_post_integral()) {
          double s=0;
          double p   = double(image.info(Ami::EntryImage::Pedestal));
          for(unsigned fn=nframes[q]; fn<nframes[q+1]; fn++) {
            int xlo(0), xhi(3000), ylo(0), yhi(3000);
            if (image.desc().xy_bounds(xlo, xhi, ylo, yhi, fn)) {
              for(int j=ylo; j<yhi; j++)
                for(int i=xlo; i<xhi; i++) {
                  double v = double(image.content(i,j))-p;
                  s += v;
                }
            }
          }
          sum += s;
          }
        }
#ifdef _OPENMP
      }
#endif

      if (image.desc().options()&CspadCalib::option_post_integral())
        cache.cache(_feature[80],sum);

      return true;
    }
    void fill(Ami::EntryImage& image,
              double v0, double v1) const
    {
      for(unsigned iq=0; iq<4; iq++)
        if (_config.quadMask()&(1<<iq))
          quad[iq]->fill(image,_config.roiMask(iq),v0,v1);
    }
    void set_pedestals(FILE* f)
    {
      for(unsigned iq=0; iq<4; iq++)
        quad[iq]->set_pedestals(f);
    }
    void rename(const char* s)
    {
      char buff[64];
      unsigned qmask = _config.quadMask();
      for(unsigned i=0; i<4; i++)
        if (qmask & (1<<i)) {
          for(unsigned a=0; a<4; a++) {
            sprintf(buff,"%s:Quad[%d]:Temp[%d]",s,i,a);
            _hdl._rename_cache(_feature[20*i+a],buff);
          }
          if (Ami::EventHandler::post_diagnostics())
            for(unsigned a=0; a<16; a++) {
              sprintf(buff,"%s:Quad[%d]:CommonMode[%d]",s,i,a);
              _hdl._rename_cache(_feature[20*i+a+4],buff);
            }
        }

      sprintf(buff,"%s:Sum",s);
      _hdl._rename_cache(_feature[80],buff);
    }
    unsigned ppb() const { return _ppb; }
    unsigned xpixels() { return _pixels; }
    unsigned ypixels() { return _pixels; }
  private:
    Ami::EventHandlerF& _hdl;
    Quad* quad[4];
    const Src&  _src;
    ConfigCache _config;
    enum { NumFeatures=81 };
    mutable int _feature[NumFeatures];
    unsigned _ppb;
    unsigned _pixels;
    const CspadTemp& _therm;
  };

  class CspadPFF : public Ami::PeakFinderFn {
  public:
    CspadPFF(FILE* gain,
             FILE* rms,
             const Detector*  detector,
             Ami::DescImage& image) :
      _detector(*detector),
      _nbinsx  (image.nbinsx()),
      _nbinsy  (image.nbinsy()),
      _values  (new Ami::EntryImage(image))
    {
      image.pedcalib (true);
      image.gaincalib(gain!=0);
      image.rmscalib (rms !=0);
    }
    virtual ~CspadPFF()
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
    Ami::PeakFinderFn* clone() const { return new CspadPFF(*this); }
  private:
    CspadPFF(const CspadPFF& o) :
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

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_CspadElement);
  types.push_back(Pds::TypeId::Id_CspadCompressedElement);
  return types;
}

CspadHandler::CspadHandler(const Pds::DetInfo& info, FeatureCache& features) :
  EventHandlerF(info, data_type_list(), Pds::TypeId::Id_CspadConfig, features),
  _entry(0),
  _unbinned_entry(0),
  _detector(0),
  _unbinned_detector(0),
  _options   (0),
  _therm     (RDIV)
{
  unsigned s = sizeof(CspadGeometry::off_no_ped)/sizeof(int16_t);
  for (unsigned i=0; i<s; i++) {
    CspadGeometry::off_no_ped[i] = (int16_t)Offset;
    CspadGeometry::fgn_no_ped[i] = 1.;
  }
}

CspadHandler::~CspadHandler()
{
  if (_detector)
    delete _detector;
  if (_unbinned_detector)
    delete _unbinned_detector;
}

unsigned CspadHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* CspadHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

const Entry* CspadHandler::hidden_entry(unsigned i) const { return i==0 ? _unbinned_entry : 0; }

void CspadHandler::rename(const char* s)
{
  if (_entry) {
    _entry->desc().name(s);
    _detector->rename(s);
  }
}

void CspadHandler::reset() { _entry = 0; _unbinned_entry = 0; EventHandlerF::reset(); }

void CspadHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
  //
  //  Load pedestals
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];

  DetInfo dInfo(info_mask());
  FILE *f = Calib::fopen(dInfo, "ped", "pedestals");
  FILE *s = Calib::fopen(dInfo, "sta", "pixel_status");

  sprintf(oname1,"gain.%08x.dat",dInfo.phy());
  sprintf(oname2,"/reg/g/pcds/pds/cspadcalib/gain.%08x.dat",dInfo.phy());
  FILE *g = Calib::fopen_dual(oname1, oname2, "gain map");

  FILE *rms = Calib::fopen(dInfo, "res", "pixel_rms");

  CspadGeometry::ConfigCache cfg(type,payload);

  _create_entry( cfg,f,s,g,rms,
                 _detector, _entry, resolution());
#ifndef UNBINNED
  _create_entry( cfg,f,s,g,rms,
                 _unbinned_detector, _unbinned_entry, 1<<12);
#endif

  Ami::PeakFinder::register_(info().phy(),   
                             new CspadGeometry::CspadPFF(g,rms,_detector,_entry->desc()));

  if (f ) fclose(f);
  if (s ) fclose(s);
  if (g ) fclose(g);
  if (rms) fclose(rms);
}

void CspadHandler::_create_entry(const CspadGeometry::ConfigCache& cfg,
                                 FILE* f, FILE* s, FILE* g, FILE* rms, 
                                 CspadGeometry::Detector*& detector,
                                 EntryImage*& entry, 
                                 unsigned max_pixels) 
{
  if (f ) rewind(f);
  if (s ) rewind(s);
  if (g ) rewind(g);
  if (rms) rewind(rms);

  if (detector)
    delete detector;

  detector = new CspadGeometry::Detector(*this,info_mask(),cfg,f,s,g,rms,max_pixels,_full_resolution(),_therm);

  const unsigned ppb = detector->ppb();
  const DetInfo& det = static_cast<const DetInfo&>(info());
  DescImage desc(det, (unsigned)0, ChannelID::name(det,0),
                 detector->xpixels()/ppb, detector->ypixels()/ppb, 
                 ppb, ppb);
  desc.set_scale(pixel_size*1e-3,pixel_size*1e-3);
    
  detector->fill(desc);

  entry = new EntryImage(desc);
  memset(entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));

  entry->info(Offset*ppb*ppb,EntryImage::Pedestal);

  entry->info(0,EntryImage::Normalization);
  entry->invalid();
}


void CspadHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void CspadHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  const Xtc* xtc = reinterpret_cast<const Xtc*>(payload)-1;

  _event(xtc->contains, xtc->payload(), xtc->sizeofPayload(), t);
}

void CspadHandler::_event    (TypeId contains, const char* payload, size_t sizeofPayload, const Pds::ClockTime& t)
{
  if (_entry) {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("CspadHandler::event options %x -> %x\n", _options, o);
      _options = o;
    }

    if (_entry->desc().options() & CspadCalib::option_reload_pedestal()) {
      DetInfo dInfo(info_mask());
      FILE *f = Calib::fopen(dInfo, "ped", "pedestals", true);
      if (f) {
	_detector->set_pedestals(f);
	_entry->desc().options( _entry->desc().options()&~CspadCalib::option_reload_pedestal() );
	fclose(f);
      }
      else {
	printf("CspadHandler pedestal reload failed\n");
	_entry->desc().options( _entry->desc().options()&~CspadCalib::option_reload_pedestal() );
      }
    }
    
    if (_detector->fill(*_entry,_cache,contains,payload,sizeofPayload)) {
      _entry->info(1,EntryImage::Normalization);
      _entry->valid(t);
    }
    else {
      _entry->invalid(); 
    }
  }
  if (_unbinned_entry && _unbinned_entry->desc().used()) {
    _unbinned_detector->fill(*_unbinned_entry,_cache,contains,payload,sizeofPayload);
    _unbinned_entry->info(1,EntryImage::Normalization);
    _unbinned_entry->valid(t);
  }
}

void CspadHandler::_damaged() 
{
  if (_entry) 
    _entry->invalid(); 
  if (_unbinned_entry) 
    _unbinned_entry->invalid(); 
}

