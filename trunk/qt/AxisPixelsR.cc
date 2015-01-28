#include "ami/qt/AxisPixelsR.hh"

using namespace Ami::Qt;

int AxisPixelsR::ftick(double p) const { 
  if (p<_xlo) return int(_xlo);
  if (p>=_xhi) return int(_xhi)-1;
  return int(p-_xlo); 
}

int AxisPixelsR::ftick_u(double p) const { return int(p-_xlo); }

