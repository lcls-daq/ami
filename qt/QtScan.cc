#include "QtScan.hh"
#include "ami/qt/AxisArray.hh"
#include "ami/qt/Defaults.hh"

#include "ami/data/AbsEval.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/EntryScan.hh"

#include "qwt_plot.h"
#include "qwt_symbol.h"

using namespace Ami::Qt;


QtScan::QtScan(const QString&   title,
	       const Ami::EntryScan& entry,
	       const AbsTransform& x,
	       const AbsTransform& y,
	       const QColor& c,
               int symbol_size,
               int symbol_style) :
  QtBase(title,entry),
  _xscale(x),
  _yscale(y),
  _curve(entry.desc().name()),
  _eval (AbsEval::lookup(entry.desc().stat()))
{
  if (entry.desc().scatter()) {
    _curve.setStyle(QwtPlotCurve::Dots);
    QBrush b(c);
    QPen   p(c);
    QwtSymbol symbol(QwtSymbol::Style(symbol_style),
                     b,p,
                     QSize(symbol_size,symbol_size));
    _curve.setSymbol(symbol);
  }
  else {
    _curve.setStyle(QwtPlotCurve::Lines);
  }
  _curve.setPen  (QPen(c));
  //  _curve.setCurveAttribute(QwtPlotCurve::Inverted,true);
  
  unsigned nb = entry.desc().nbins();
  _xa = new double[nb];
  _x  = new double[nb];
  _y  = new double[nb];
  unsigned b=0;
  unsigned i=0;

  while(b<nb) {
    _xa[b] = _xscale(entry.xbin(b));
    if (entry.nentries(b)) {
      _x[i] = _xscale(entry.xbin(b));
      _y[i] = _eval->evaluate(entry,b);
      i++;
    }
    b++;
  }
  _curve.setRawData(_x,_y,i);  // QwtPlotCurve wants the x-endpoint
  _xinfo = new AxisArray(_xa,nb);
}
  
  
QtScan::~QtScan()
{
  _curve.attach(NULL);
  delete _xinfo;
  delete[] _xa;
  delete[] _x;
  delete[] _y;
  delete   _eval;
}

void           QtScan::dump  (FILE* f) const
{
  int prec = Defaults::instance()->save_precision();
  for(unsigned b=0; b<_curve.data().size(); b++) {
    double x = _x[b];
    double y = _y[b];
    fprintf(f,"%.*g %.*g\n",
            prec,x,
            prec,y);
  }
}

void           QtScan::attach(QwtPlot* p)
{
  _curve.attach(p);
  if (p) {
    const EntryScan& _entry = static_cast<const EntryScan&>(entry());
    p->setAxisTitle(QwtPlot::xBottom,_entry.desc().xtitle());
    p->setAxisTitle(QwtPlot::yLeft  ,_entry.desc().ytitle());
  }
}

void           QtScan::update()
{
  if (!entry().valid()) {
#ifdef DBUG
    printf("QtScan::update invalid\n");
#endif
    return;
  }

  const EntryScan& _entry = static_cast<const EntryScan&>(entry());
  unsigned nb = _entry.desc().nbins();
  unsigned i=0;
  for(unsigned b=0; b<nb; b++)
    if (_entry.nentries(b)) {
      _x[i] = _xscale(_entry.xbin(b));
      _y[i] = _eval->evaluate(_entry,b);
      i++;
    }
  _curve.setRawData(_x,_y,i);  // QwtPlotCurve wants the x-endpoint
}

void QtScan::xscale_update()
{
  update();
}

void QtScan::yscale_update()
{
  update();
}

const AxisInfo* QtScan::xinfo() const
{
  return _xinfo;
}

void QtScan::set_color(unsigned c)
{
  QPen pen(_curve.pen());
  pen.setColor(color(c));
  _curve.setPen(pen);

  QwtSymbol symbol(_curve.symbol());
  symbol.setPen(QPen(color(c)));
  symbol.setBrush(QBrush(color(c)));
  _curve.setSymbol(symbol);
}

QColor QtScan::get_color() const { return _curve.pen().color(); }
