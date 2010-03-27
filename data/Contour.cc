#include "ami/data/Contour.hh"

using namespace Ami;

Contour::Contour() :
  _n(0)
{
  unsigned i=0;
  while(i<=MaxOrder)
    _c[i++] = 0;
}

Contour::Contour(float* f, unsigned n) :
  _n(n)
{
  unsigned i=0;
  while(i<n) {
    _c[i] = f[i];
    i++;
  }
  while(i<=MaxOrder)
    _c[i++] = 0;
}

Contour::Contour(const Contour& p) :
  _n(p._n)
{
  unsigned i=0;
  while(i<=MaxOrder) {
    _c[i] = p._c[i];
    i++;
  }
}

Contour::~Contour() {}

float Contour::value(float x) const {
  float y=0;
  float xn=1;
  for(unsigned i=0; i<_n; i++) {
    y += _c[i]*xn;
    xn *= x;
  }
  return y;
}

void  Contour::extremes(double  x0,double  x1,
			double& y0,double& y1) const
{
  double x  = x0;
  double ymin=value(x), ymax=ymin;
  while (++x<=x1) {
    double y = value(x);
    if (y < ymin) { ymin = y; }
    if (y > ymax) { ymax = y; }
  }
  y0 = ymin;
  y1 = ymax;
}
