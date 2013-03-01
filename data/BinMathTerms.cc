#include "ami/data/BinMathTerms.hh"

#include "ami/data/DescWaveform.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/ImageMask.hh"

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
  switch(_mom) {
  case First:
    {
      double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
      double x  = desc.xlow()+(double(lo)+0.5)*dx;
      for(unsigned i=lo; i<=hi; i++, x+=dx)
        sum += e->content(i)*x;
    }
    break;
  case Second:
    {
      double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
      double x  = desc.xlow()+(double(lo)+0.5)*dx;
      for(unsigned i=lo; i<=hi; i++, x+=dx)
        sum += e->content(i)*x*x;
    }
    break;
  default:
    for(unsigned i=lo; i<=hi; i++)
      sum += e->content(i);
    break;
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
  switch(_mom) {
  case First:
    {
      double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
      double x  = desc.xlow()+(double(lo)+0.5)*dx;
      for(unsigned i=lo; i<=hi; i++, x+=dx)
        sum += e->content(i)*x;
    }
    break;
  case Second:
    {
      double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
      double x  = desc.xlow()+(double(lo)+0.5)*dx;
      for(unsigned i=lo; i<=hi; i++, x+=dx)
        sum += e->content(i)*x*x;
    }
    break;
  default:
    for(unsigned i=lo; i<=hi; i++)
      sum += e->content(i);
    break;
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
  switch(_mom) {
  case First:
    {
      double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
      double x  = desc.xlow()+(double(lo)+0.5)*dx;
      for(unsigned i=lo; i<=hi; i++, x+=dx)
        sum += e->ymean(i)*x;
    }
    break;
  case Second:
    {
      double dx = (desc.xup()-desc.xlow())/double(desc.nbins());
      double x  = desc.xlow()+(double(lo)+0.5)*dx;
      for(unsigned i=lo; i<=hi; i++, x+=dx)
        sum += e->ymean(i)*x*x;
    }
    break;
  default:
    for(unsigned i=lo; i<=hi; i++)
      sum += e->ymean(i);
    break;
  }
  return sum; 
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
  double s0 = 0, sum = 0, sqsum = 0;
  double xsum = 0, ysum = 0;
  double p   = double(e.info(EntryImage::Pedestal));
  if (mask) {
    unsigned xlo=_xlo, xhi=_xhi, ylo=_ylo, yhi=_yhi;
    if (xhi >= d.nbinsx()) xhi = d.nbinsx()-1;
    if (yhi >= d.nbinsy()) yhi = d.nbinsy()-1;
    for(unsigned j=ylo; j<=yhi; j++) {
      if (!mask->row(j)) continue;
      for(unsigned i=xlo; i<=xhi; i++)
        if (mask->rowcol(j,i)) {
          double v = double(e.content(i,j))-p;
          s0    += 1;
          sum   += v;
          sqsum += v*v;
	  xsum  += double(i)*v;	    
	  ysum  += double(j)*v;
        }
    }
  }
  else if (d.nframes()) {
    for(unsigned fn=0; fn<d.nframes(); fn++) {
      int xlo(_xlo), xhi(_xhi+1), ylo(_ylo), yhi(_yhi+1);
      if (d.xy_bounds(xlo, xhi, ylo, yhi, fn)) {
        for(int j=ylo; j<yhi; j++)
          for(int i=xlo; i<xhi; i++) {
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
  else {
    unsigned xlo=_xlo, xhi=_xhi, ylo=_ylo, yhi=_yhi;
    if (xhi >= d.nbinsx()) xhi = d.nbinsx()-1;
    if (yhi >= d.nbinsy()) yhi = d.nbinsy()-1;
    for(unsigned j=ylo; j<=yhi; j++)
      for(unsigned i=xlo; i<=xhi; i++) {
        double v = double(e.content(i,j))-p;
        s0    += 1;
        sum   += v;
        sqsum += v*v;
	xsum  += double(i)*v;	    
	ysum  += double(j)*v;
      }
  }
  double n = double(e.info(EntryImage::Normalization));
  double q = double(d.ppxbin()*d.ppybin());
  double v;
  switch(_mom) {
  case Zero:
    v = sum;
    if (n>0) v/=n;
    break;
  case Mean:
    v = sum/(s0*q);
    if (n>0) v/=n;
    break;
  case Variance:
    v = sqrt((sqsum/s0 - pow(sum/s0,2))/q);;
    if (n>0) v/=n;
    break;
  case Contrast:
    v = sqrt(s0*sqsum/(sum*sum) - 1);
    //          if (n>0) v/=sqrt(n);
    break;
  case XCenterOfMass:
    v = xsum/sum*d.ppxbin();
    break;
  case YCenterOfMass:
    v = ysum/sum*d.ppybin();
    break;
  default:
    v = 0;
    break;
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
      for(int j=iylo; j<=iyhi; j++) {
        if (!mask->row(j)) continue;
        double dy  = d.biny(j)-yc;
        double dy2 = dy*dy;
        for(int i=ixlo; i<=ixhi; i++) {
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
        for(int j=iylo; j<=iyhi; j++) {
          double dy  = d.biny(j)-yc;
          double dy2 = dy*dy;
          for(int i=ixlo; i<=ixhi; i++) {
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
      for(int j=iylo; j<=iyhi; j++) {
        double dy  = d.biny(j)-yc;
        double dy2 = dy*dy;
        for(int i=ixlo; i<=ixhi; i++) {
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
    v = xsum/sum*d.ppxbin();
    break;
  case YCenterOfMass:
    v = ysum/sum*d.ppybin();
    break;
  default:
    v = 0;
    break;
  }
  return v;
}
