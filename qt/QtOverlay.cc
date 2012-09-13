#include "ami/qt/QtOverlay.hh"

#include "ami/qt/OverlayParent.hh"
#include "ami/qt/QtPlot.hh"

using namespace Ami::Qt;

QtOverlay::QtOverlay(OverlayParent& parent) :  _parent(parent) 
{
}

QtOverlay::~QtOverlay()
{
  _parent.remove_overlay(this);
}

void QtOverlay::attach(QtPlot& p)
{
  p.add_overlay(this);
}
