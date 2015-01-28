#include "AxisBins.hh"

#include <stdio.h>

using namespace Ami::Qt;

int    AxisBins::tick    (double p ) const {
  if (p < _xlo) return 0;
  if (p > _xhi) return _n;
  return int(double(_n)*(p-_xlo)/(_xhi-_xlo));
}

int    AxisBins::tick_u  (double p ) const {
  return int(double(_n)*(p-_xlo)/(_xhi-_xlo));
}

//  Interpolated position [_xlo,_xhi]
double AxisBins::position(int    i) const { 
  return (_xlo*double(_n-i) + _xhi*double(i))/double(_n); 
}
