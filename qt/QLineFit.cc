#include "ami/qt/QLineFit.hh"
#include "ami/data/LineFitEntry.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"

#include "qwt_plot.h"

using Ami::LineFit;
using Ami::LineFitEntry;
using namespace Ami::Qt;

static QColor _color[]  = { QColor(129  , 0, 0),
			    QColor(255, 129, 0),
			    QColor(129, 129, 0) };

QLineFitEntry::QLineFitEntry(LineFit::Method m,
			     QwtPlot* frame) :
  _name (LineFit::method_str(m)),
  _fit  (LineFitEntry::instance(m)),
  _curve(QString(_name.c_str()))
{
  QColor c();
  _curve.attach(frame);
  _curve.setStyle(QwtPlotCurve::Lines);
  _curve.setPen  (QPen(_color[m]));
  _curve.setCurveAttribute(QwtPlotCurve::Inverted,true);
  _x[0]=_x[1]=_y[0]=_y[1]=0;
  _curve.setRawData(_x,_y,2);
}

void QLineFitEntry::attach(QwtPlot* f) { _curve.attach(f); }

void QLineFitEntry::fit(const Entry& p)
{
  double slope=0,intercept=0,x0=0,x1=0;
  switch(p.desc().type()) {
  case DescEntry::Prof: 
    _fit->fit(slope,intercept,x0,x1,
	      static_cast<const EntryProf&>(p));
    break;
  case DescEntry::Scan: 
    _fit->fit(slope,intercept,x0,x1,
	      static_cast<const EntryScan&>(p));
    break;
  default:
    break;
  }
      
  _x[0]= x0;
  _x[1]= x1;
  _y[0]= slope*x0+intercept; 
  _y[1]= slope*x1+intercept; 

  _curve.setTitle( QString("%1[%2,%3]")
		   .arg(_name.c_str()).arg(slope).arg(intercept) );
}

QLineFit::QLineFit() : _frame(0) 
{
  for(unsigned i=0; i<LineFit::NumberOf; i++)
    _fits[LineFit::Method(i)] = 0;
}

QLineFit::~QLineFit() 
{
  for(unsigned i=0; i<LineFit::NumberOf; i++)
    if (_fits[LineFit::Method(i)])
      delete _fits[LineFit::Method(i)];
}

void QLineFit::attach(QwtPlot* frame) 
{
  _frame=frame; 
  for(unsigned i=0; i<LineFit::NumberOf; i++) {
    QLineFitEntry* e = _fits[LineFit::Method(i)];
    if (e) e->attach(frame);
  }
}

void QLineFit::show_fit(LineFit::Method m, bool show) 
{ 
  QLineFitEntry* e = _fits[m];
  if (show) {
    if (e==0)
      _fits[m] = new QLineFitEntry(m, _frame);
  }
  else if (e) {
    delete e;
    _fits[m]=0;
  }
}

void QLineFit::update_fit(const Entry& p)
{
  for(unsigned i=0; i<LineFit::NumberOf; i++) {
    QLineFitEntry* e = _fits[LineFit::Method(i)];
    if (e && _frame)
      e->fit(p);
  }
}

QLineFitMenu::QLineFitMenu(QLineFit& host) :
  QMenu("LineFit")
{
  for(unsigned i=0; i<LineFit::NumberOf; i++)
    addAction(new QLineFitAction(host,LineFit::Method(i)));
}

QLineFitMenu::~QLineFitMenu() {}

QLineFitAction::QLineFitAction(QLineFit&       host,
			       LineFit::Method m) :
  QAction(LineFit::method_str(m),0),
  _host  (host),
  _method(m)
{
  setCheckable(true);
  connect(this, SIGNAL(changed()), this, SLOT(show_fit()));
}

QLineFitAction::~QLineFitAction() {}

void QLineFitAction::show_fit() 
{ 
  _host.show_fit(_method, isChecked()); 
}
