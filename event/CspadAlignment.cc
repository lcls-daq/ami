#include "ami/event/CspadAlignment.hh"

#include <stdio.h>

using namespace Ami::Cspad;

enum Rotation { D0, D90, D180, D270, NPHI=4 };

static void _transform(double& x,double& y,double dx,double dy,Rotation r)
{
  switch(r) {
  case D0  :    x += dx; y += dy; break;
  case D90 :    x += dy; y -= dx; break;
  case D180:    x -= dx; y -= dy; break;
  case D270:    x -= dy; y += dx; break;
  default:                        break;
  }
}

static const double xAsicOffset = 1.0e-3;
static const double yAsicOffset = 1.0e-3;
// static const double xAsicOffset = 0;
// static const double yAsicOffset = 0;
static const double asicWidth   = 21.335e-3;
static const double asicHeight  = 22.488e-3;

TwoByTwoAlignment QuadAlignment::twobytwo(unsigned i) const
{
  static const Rotation r[] = { D270, D180, D90, D180 };

  int pOrigin = 2*i+1;

  double px0(_twobytwo[pOrigin]._pad.x*1.e-6);
  double py0(_twobytwo[pOrigin]._pad.y*1.e-6);
  double px1(_twobytwo[pOrigin-1]._pad.x*1.e-6);
  double py1(_twobytwo[pOrigin-1]._pad.y*1.e-6);

  TwoByTwoAlignment a;
  a.xOrigin = 0;
  a.yOrigin = 0;
  _transform(a.xOrigin, a.yOrigin, px0, py0, D270);

  double x0(xAsicOffset), y0(yAsicOffset);
  a.xAsicOrigin[0] = x0;
  a.yAsicOrigin[0] = y0;
  a.xAsicOrigin[1] = x0;
  a.yAsicOrigin[1] = y0+asicWidth;

  _transform(x0, y0, px1-px0, py1-py0, r[i]);

  a.xAsicOrigin[2] = x0;
  a.yAsicOrigin[2] = y0;
  a.xAsicOrigin[3] = x0;
  a.yAsicOrigin[3] = y0+asicWidth;

  return a;
}
