#include "ami/qt/AxisPixels.hh"

using namespace Ami::Qt;

int AxisPixels::tick(double p) const { return AxisBins::tick(p+0.5); }

int AxisPixels::tick_u(double p) const { return AxisBins::tick_u(p+0.5); }

