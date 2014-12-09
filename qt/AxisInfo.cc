#include "ami/qt/AxisInfo.hh"

using namespace Ami::Qt;

int AxisInfo::tick_u(double pos) const { return tick(pos); }

int AxisInfo::ftick(double pos) const { return tick(pos); }

int AxisInfo::ftick_u(double pos) const { return tick_u(pos); }
