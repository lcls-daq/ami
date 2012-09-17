#include "QtEmpty.hh"
#include "ami/qt/AxisBins.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/EntryScalar.hh"

#include "qwt_plot.h"

using namespace Ami::Qt;

static Ami::DescScalar  sdesc ("empty","");
static Ami::EntryScalar sentry(sdesc);

QtEmpty::QtEmpty() :
  QtBase(QString("empty"),sentry),
  _curve(sdesc.name())
{
  _xinfo = new AxisBins(0,1,1);
}
  
  
QtEmpty::~QtEmpty()
{
  _curve.attach(NULL);
  delete _xinfo;
}

void           QtEmpty::dump  (FILE* f) const
{
}

void           QtEmpty::attach(QwtPlot* p)
{
  _curve.attach(p);
}

void           QtEmpty::update()
{
}

void QtEmpty::xscale_update()
{
  update();
}

void QtEmpty::yscale_update()
{
  update();
}

const AxisInfo* QtEmpty::xinfo() const
{
  return _xinfo;
}

void QtEmpty::set_color(unsigned c)
{
  QPen pen(_curve.pen());
  pen.setColor(color(c));
  _curve.setPen(pen);
}
