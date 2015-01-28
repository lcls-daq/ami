#include "ami/data/BinMathTerms.hh"

#include "ami/data/DescWaveform.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/ImageMask.hh"
#include "ami/data/VectorArray.hh"

#include "psalg/psalg.h"

#define DBUG

using namespace Ami;

BinMathC::EntryWaveformTerm::EntryWaveformTerm(const Entry*& e, unsigned lo, unsigned hi,
                                               Moment mom) :
  _entry(e), _lo(lo), _hi(hi), _mom(mom) {}


double BinMathC::EntryWaveformTerm::evaluate() const
{ double sum=0;
  unsigned lo=_lo, hi=_hi;
  const EntryWaveform* e = static_cast<const EntryWaveform*>(_entry);
  unsigned offset = 0;
  const DescWaveform& desc = e->desc();
  /*
   * xlow is 0.0 for all regular waveforms.  However, for references read from a .dat file,
   * the X values are the *middle* of the bins, and do not start at 0.0!  (Because of this,
   * we use floor below instead of round in calculating the offset.)
   */
  if (desc.xlow() != 0.0) {
    offset = (int) floor(desc.xlow() * (desc.nbins() - 1) / (desc.xup() - desc.xlow()));
    lo -= offset;
    hi -= offset;
  }

  double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
  double x0 = desc.xlow()+(double(lo)+0.5)*dx;
  ndarray<const double,1> a = make_ndarray(e->content()+lo,hi-lo+1);
  ndarray<double,1> m = psalg::moments(a, x0, dx);

  switch(_mom) {
  case First : sum = m[1]; break;
  case Second: sum = m[2]; break;
  default    : sum = m[0]; break;
  }

  double n = e->info(EntryWaveform::Normalization);
  return n > 0 ? sum / n : sum; 
}

BinMathC::EntryTH1FTerm::EntryTH1FTerm(const Entry*& e, unsigned lo, unsigned hi,
                                       Moment mom) :
  _entry(e), _lo(lo), _hi(hi), _mom(mom) {}

double BinMathC::EntryTH1FTerm::evaluate() const
{ double sum=0;
  unsigned lo=_lo, hi=_hi;
  const EntryTH1F* e = static_cast<const EntryTH1F*>(_entry);
  const DescTH1F& desc = e->desc();

  double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
  double x0 = desc.xlow()+(double(lo)+0.5)*dx;
  ndarray<const double,1> a = make_ndarray(e->content()+lo,hi-lo+1);
  ndarray<double,1> m = psalg::moments(a, x0, dx);

  switch(_mom) {
  case First : sum = m[1]; break;
  case Second: sum = m[2]; break;
  default    : sum = m[0]; break;
  }

  double n = e->info(EntryTH1F::Normalization);
  return n > 0 ? sum / n : sum; 
}

BinMathC::EntryProfTerm::EntryProfTerm(const Entry*& e, unsigned lo, unsigned hi,
                                       Moment mom) :
  _entry(e), _lo(lo), _hi(hi), _mom(mom) {}

double BinMathC::EntryProfTerm::evaluate() const
{ double sum=0;
  unsigned lo=_lo, hi=_hi;
  const EntryProf* e = static_cast<const EntryProf*>(_entry);
  const DescProf& desc = e->desc();

  double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
  double x0 = desc.xlow()+(double(lo)+0.5)*dx;
  ndarray<const double,1> a = make_ndarray(e->ysum   ()+lo,hi-lo+1);
  ndarray<const double,1> b = make_ndarray(e->entries()+lo,hi-lo+1);
  ndarray<double,1> m = psalg::moments(a, b, x0, dx);

  switch(_mom) {
  case First : sum = m[1]; break;
  case Second: sum = m[2]; break;
  default    : sum = m[0]; break;
  }

  return sum; 
}

BinMathC::VASizeTerm::VASizeTerm(const Entry*& e) : _entry(e) {}

double BinMathC::VASizeTerm::evaluate() const
{
  const EntryRef* e = static_cast<const EntryRef*>(_entry);
  return reinterpret_cast<const VectorArray*>(e->data())->nentries();
}


BinMathC::VAElementTerm::VAElementTerm(const Entry*& e, const unsigned& i, unsigned element) :
  _entry(e), _index(i), _element(element) {}

double BinMathC::VAElementTerm::evaluate() const
{
  const EntryRef* e = static_cast<const EntryRef*>(_entry);
  return reinterpret_cast<const VectorArray*>(e->data())->element(_element)[_index];
}


BinMathC::EntryImageTerm::EntryImageTerm(const Entry*& e, 
                                         unsigned xlo, unsigned xhi, 
                                         unsigned ylo, unsigned yhi,
                                         Moment mom) :
  _entry(e), _xlo(xlo), _xhi(xhi), _ylo(ylo), _yhi(yhi), _mom(mom) 
{
  switch(_mom) {
  case First: 
    printf("EntryImageTerm first moment not implemented\n"); 
    break;
  case Second: 
    printf("EntryImageTerm second moment not implemented\n"); 
    break;
  default:
    break;
  }
}

double BinMathC::EntryImageTerm::evaluate() const {
  const EntryImage& e = *static_cast<const EntryImage*>(_entry);
  const DescImage&  d = e.desc();
  const ImageMask* mask = d.mask();

  unsigned shape[] = {d.nbinsy(),d.nbinsx()};
  ndarray<const unsigned,2> a(e.contents(),shape);

  unsigned mshape[] = {5};
  ndarray<double,1> m(mshape);
  double p   = double(e.info(EntryImage::Pedestal));
  if (mask) {
    unsigned xlo=_xlo, xhi=_xhi, ylo=_ylo, yhi=_yhi;
    if (xhi >= d.nbinsx()) xhi = d.nbinsx()-1;
    if (yhi >= d.nbinsy()) yhi = d.nbinsy()-1;

    unsigned bounds[][2] = { {ylo, yhi+1}, {xlo, xhi+1} };
    m = psalg::moments(a, mask->row_mask(), mask->all_mask(), p, bounds);
  }
  else if (d.nframes()) {
    for(unsigned j=0; j<5; j++)
      m[j] = 0;
    for(unsigned fn=0; fn<d.nframes(); fn++) {
      int xlo(_xlo), xhi(_xhi), ylo(_ylo), yhi(_yhi);
      if (d.xy_bounds(xlo, xhi, ylo, yhi, fn)) {
        unsigned bounds[][2] = { {ylo, yhi}, {xlo, xhi} };
        ndarray<double,1> fm = psalg::moments(a, p, bounds);
        for(unsigned j=0; j<5; j++)
          m[j] += fm[j];
      }
    }
  }
  else {
    unsigned xlo=_xlo, xhi=_xhi, ylo=_ylo, yhi=_yhi;
    if (xhi >= d.nbinsx()) xhi = d.nbinsx()-1;
    if (yhi >= d.nbinsy()) yhi = d.nbinsy()-1;

    unsigned bounds[][2] = { {ylo, yhi+1}, {xlo, xhi+1} };
    m = psalg::moments(a, p, bounds);
  }

  double n = double(e.info(EntryImage::Normalization));
  double q = double(d.ppxbin()*d.ppybin());
  if (n>0) q*=n;
  double v;
  switch(_mom) {
  case Zero         : v = m[1]; break;
  case Mean         : v = m[1]/(m[0]*q); break;
  case Variance     : v = sqrt((m[2]/m[0] - pow(m[1]/m[0],2))/q); break;
  case Contrast     : v = sqrt(m[0]*m[2]/(m[1]*m[1]) - 1); break;
  case XCenterOfMass: v = m[3]/m[1]*d.ppxbin() + d.xlow(); break;
  case YCenterOfMass: v = m[4]/m[1]*d.ppybin() + d.ylow(); break;
  default           : v = 0; break;
  }
  return v;
}

BinMathC::EntryImageTermF::EntryImageTermF(const Entry*& e, 
                                           double xc, double yc, 
                                           double r0, double r1, 
                                           double f0, double f1,
                                           Moment mom) :
  _entry(e), _xc(xc), _yc(yc), _r0(r0), _r1(r1), _f0(f0), _f1(f1), _mom(mom) 
{
  switch(_mom) {
  case First: 
    printf("EntryImageTermF first moment not implemented\n"); 
    break;
  case Second: 
    printf("EntryImageTermF second moment not implemented\n"); 
    break;
  default:
    break;
  }
}

double BinMathC::EntryImageTermF::evaluate() const 
{
  const EntryImage& e = *static_cast<const EntryImage*>(_entry);
  const DescImage& d = e.desc();
  const ImageMask* mask = d.mask();
  double s0 = 0, sum = 0, sqsum = 0;
  double xsum = 0, ysum = 0;
  const double p = e.info(EntryImage::Pedestal);
  int ixlo, ixhi, iylo, iyhi;
  if (mask) {
    if (d.rphi_bounds(ixlo, ixhi, iylo, iyhi,
                      _xc, _yc, _r1)) {
      double xc(_xc), yc(_yc);
      double r0sq(_r0*_r0), r1sq(_r1*_r1);
      for(int j=iylo; j<iyhi; j++) {
        if (!mask->row(j)) continue;
        double dy  = d.biny(j)-yc;
        double dy2 = dy*dy;
        for(int i=ixlo; i<ixhi; i++) {
          if (!mask->rowcol(j,i)) continue;
          double dx  = d.binx(i)-xc;
          double dx2 = dx*dx;
          double rsq = dx2 + dy2;
          double f   = atan2(dy,dx);
          if ( (rsq >= r0sq && rsq <= r1sq) &&
               ( (f>=_f0 && f<=_f1) ||
                 (f+2*M_PI <= _f1) ) ) {
            double v = double(e.content(i,j))-p;
            s0    += 1;
            sum   += v;
            sqsum += v*v;
	    xsum  += double(i)*v;	    
	    ysum  += double(j)*v;
          }
        }
      }
    }
  }
  else if (d.nframes()) {
    for(unsigned fn=0; fn<d.nframes(); fn++)
      if (d.rphi_bounds(ixlo, ixhi, iylo, iyhi,
                        _xc, _yc, _r1, fn)) {
        double xc(_xc), yc(_yc);
        double r0sq(_r0*_r0), r1sq(_r1*_r1);
        for(int j=iylo; j<iyhi; j++) {
          double dy  = d.biny(j)-yc;
          double dy2 = dy*dy;
          for(int i=ixlo; i<ixhi; i++) {
            double dx  = d.binx(i)-xc;
            double dx2 = dx*dx;
            double rsq = dx2 + dy2;
            double f   = atan2(dy,dx);
            if ( (rsq >= r0sq && rsq <= r1sq) &&
                 ( (f>=_f0 && f<=_f1) ||
                   (f+2*M_PI <= _f1) ) ) {
              double v = double(e.content(i,j))-p;
              s0    += 1;
              sum   += v;
              sqsum += v*v;
	      xsum  += double(i)*v;	    
	      ysum  += double(j)*v;
            }
          }
        }
      }
  }
  else {
    if (d.rphi_bounds(ixlo, ixhi, iylo, iyhi,
                      _xc, _yc, _r1)) {
      double xc(_xc), yc(_yc);
      double r0sq(_r0*_r0), r1sq(_r1*_r1);
      for(int j=iylo; j<iyhi; j++) {
        double dy  = d.biny(j)-yc;
        double dy2 = dy*dy;
        for(int i=ixlo; i<ixhi; i++) {
          double dx  = d.binx(i)-xc;
          double dx2 = dx*dx;
          double rsq = dx2 + dy2;
          double f   = atan2(dy,dx);
          if ( (rsq >= r0sq && rsq <= r1sq) &&
               ( (f>=_f0 && f<=_f1) ||
                 (f+2*M_PI <= _f1) ) ) {
            double v = double(e.content(i,j))-p;
            s0    += 1;
            sum   += v;
            sqsum += v*v;
	    xsum  += double(i)*v;	    
	    ysum  += double(j)*v;
          }
        }
      }
    }
    // 	  else {
    //             return 0;
    //          }
  }
  double n = double(e.info(EntryImage::Normalization));
  double v;
  switch(_mom) {
  case Zero:
    v = sum;
    if (n > 0) v/=n;
    break;
  case Mean:
    v = sum/s0;
    if (n > 0) v/=n;
    break;
  case Variance:
    v = sqrt(sqsum/s0 - pow(sum/s0,2));
    if (n > 0) v/=n;
    break;
  case Contrast:
    v = sqrt(s0*sqsum/(sum*sum) - 1);
    //    if ( n > 0) v/=sqrt(n);
    break;
  case XCenterOfMass:
    v = xsum/sum*d.ppxbin() + d.xlow();
    break;
  case YCenterOfMass:
    v = ysum/sum*d.ppybin() + d.ylow();
    break;
  default:
    v = 0;
    break;
  }
  return v;
}
