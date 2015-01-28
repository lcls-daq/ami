#include "ami/qt/QtOverlay.hh"

#include "ami/qt/OverlayParent.hh"
#include "ami/qt/QtPlot.hh"
#include "ami/data/Cds.hh"

using namespace Ami::Qt;

QtOverlay::QtOverlay(OverlayParent& parent) :  _parent(parent), _plot(0) 
{
}

QtOverlay::~QtOverlay()
{
  _parent.remove_overlay(this);
}

void QtOverlay::attach(QtPlot& p, Cds& cds)
{
  subscribe(cds);
  _plot=&p;
  p.add_overlay(this);
}

void QtOverlay::clear_payload() 
{
  _plot->del_overlay(this); 
}
