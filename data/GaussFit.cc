#include "ami/data/GaussFit.hh"

#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"

#include <math.h>

namespace Ami {
  class GaussFitStats {
  public:
    GaussFitStats() : _w(0), _wx(0), _wxx(0) {}
  public:
    void accum(double x, double w) {
      _w   += w;
      _wx  += w*x;
      _wxx += w*x*x;
    }
    std::vector<double> fit()
    {
      std::vector<double> p(3);
      p[0] = _w;
      p[1] = _wx/_w;
      p[2] = sqrt((_wxx - 2*p[1]*_wx + p[1]*p[1]*_w)/_w);
      return p;
    }
  private:
    double _w, _wx, _wxx;
  };
};

using namespace Ami;

GaussFit::GaussFit() : _params(3)
{
}

GaussFit::~GaussFit()
{
}

void GaussFit::fit(const EntryTH1F& e,double x0,double x1)
{
  GaussFitStats s;
  const DescTH1F& d = e.desc();
  double dx = (d.xup()-d.xlow())/double(d.nbins());
  double xo = d.xlow()+0.5*dx;
  unsigned i0 = x0<xo ? 0 : unsigned((x0-xo)/dx);
  unsigned i1 = x1<xo ? 0 : unsigned((x1-xo)/dx);
  if (i1 >= d.nbins()) i1=d.nbins()-1;

  if (x0==0 && x1==0) {
    i0 = 0;
    i1 = d.nbins()-1;
  }

  for(unsigned i=i0; i<=i1; i++) {
    double x = xo+double(i)*dx;
    s.accum( x, e.content(i) );
  }

  _params = s.fit();

  double a = dx*_params[0]/(sqrt(2*M_PI)*_params[2]);
  if (e.desc().isnormalized())
    a /= e.info(EntryTH1F::Normalization);

  i0 *= 2;
  i1 *= 2;
  unsigned n = i1-i0+1;
  _x.resize(n);
  _y.resize(n);
  xo  = d.xlow();
  dx *= 0.5;
  xo += dx*double(i0);
  for(unsigned i=0; i<n; i++) {
    double x = xo+double(i)*dx;
    _x[i] = x;
    x = (x - _params[1])/_params[2];
    _y[i] = a*exp(-0.5*x*x);
  }
}

void GaussFit::fit(const EntryProf& e,double x0,double x1)
{
  GaussFitStats s;
  const DescProf& d = e.desc();
  double dx = (d.xup()-d.xlow())/double(d.nbins());
  double xo = d.xlow()+0.5*dx;
  unsigned i0 = x0<xo ? 0 : unsigned((x0-xo)/dx);
  unsigned i1 = x1<xo ? 0 : unsigned((x1-xo)/dx);
  if (i1 >= d.nbins()) i1=d.nbins()-1;

  if (x0==0 && x1==0) {
    i0 = 0;
    i1 = d.nbins()-1;
  }

  for(unsigned i=i0; i<=i1; i++) {
    double x = xo+double(i)*dx;
    s.accum( x, e.ymean(i) );
  }

  _params = s.fit();

  double a = _params[0]/(sqrt(2*M_PI)*_params[2]);

  i0 *= 2;
  i1 *= 2;
  unsigned n = i1-i0+1;
  _x.resize(n);
  _y.resize(n);
  xo  = d.xlow();
  dx *= 0.5;
  xo += dx*double(i0);
  for(unsigned i=0; i<n; i++) {
    double x = xo+double(i)*dx;
    _x[i] = x;
    x = (x - _params[1])/_params[2];
    _y[i] = a*exp(-0.5*x*x);
  }
}

std::vector<double>  GaussFit::params() const { return _params; }
std::vector<QString> GaussFit::names () const {
  std::vector<QString> n;
  n.push_back(QString("A"));
  n.push_back(QString(QChar(0x3bc)));
  n.push_back(QString(QChar(0x3c3)));
  return n;
}
const std::vector<double>&  GaussFit::x() const { return _x; }
const std::vector<double>&  GaussFit::y() const { return _y; }
