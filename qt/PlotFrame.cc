#include "PlotFrame.hh"

#include "ami/qt/Cursors.hh"

#include <QtGui/QMouseEvent>

#include "qwt_plot_canvas.h"

using namespace Ami::Qt;

PlotFrame::PlotFrame(QWidget* parent) : QwtPlot(parent), _c(0) {}

PlotFrame::~PlotFrame() {}

void PlotFrame::mousePressEvent(QMouseEvent* e)
{
  QPoint p = canvas()->pos();
  int ix = e->x() - p.x();
  int iy = e->y() - p.y();
  double x = invTransform(QwtPlot::xBottom,ix);
  double y = invTransform(QwtPlot::yLeft  ,iy);
  if (_c)
    _c->set_cursor(x,y);
  QWidget::mousePressEvent(e);
}

void PlotFrame::set_cursor_input(Cursors* c) { _c=c; }
