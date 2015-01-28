#include "ami/data/LeastSquaresFit.hh"

#include "ami/data/EntryScan.hh"
#include "ami/data/EntryProf.hh"

namespace Ami {
  class LeastSquaresFitStats {
  public:
    LeastSquaresFitStats() : _n(0), _x(0), _xx(0), _y(0), _xy(0) {}
  public:
    void accum(double x, double y, double w) {
      _n  += w;
      _x  += w*x;
      _xx += w*x*x;
      _y  += w*y;
      _xy += w*x*y;
    }
    void fit(double& slope, double& intercept)
    {
      double disc = (_n*_xx - _x*_x);
      if (disc > 0) {
	slope     = (_n*_xy - _x*_y) / disc; 
	intercept = (_y - slope*_x) / _n;
      }
      else {
	slope = intercept = 0;
      }
    }
  private:
    double _n, _x, _xx, _y, _xy;
  };
};

using namespace Ami;

LeastSquaresFit::LeastSquaresFit()
{
}

LeastSquaresFit::~LeastSquaresFit()
{
}

void LeastSquaresFit::fit(double& slope, double& intercept, 
			  double& x0   , double& x1, 
			  const EntryProf& e)
{
  LeastSquaresFitStats s;
  const DescProf& d = e.desc();
  double dx = (d.xup()-d.xlow())/double(d.nbins());
  double xo = d.xlow()+0.5*dx;
  x0 = d.xup();
  for(unsigned i=0; i<d.nbins(); i++)
    if (e.nentries(i)>0) {
      double x = xo+double(i)*dx;
      s.accum( x, e.ymean(i), e.nentries(i) );
      x1 = x;
      if (x<x0) x0=x;
    }
  s.fit(slope,intercept);
}

void LeastSquaresFit::fit(double& slope, double& intercept, 
			  double& x0   , double& x1, 
			  const EntryScan& e)
{
  LeastSquaresFitStats s;
  const DescScan& d = e.desc();
  x0 = x1 = e.xbin(0);
  for(unsigned i=0; i<d.nbins(); i++) {
    double x = e.xbin(i);
    s.accum( x, e.ymean(i), e.nentries(i) );
    if (x<x0) x0=x;
    if (x>x1) x1=x;
  }
  s.fit(slope,intercept);
}

