#include "ami/qt/QLineFitEntry.hh"
#include "ami/data/LineFitEntry.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"

#include "qwt_plot.h"

using Ami::LineFit;
using Ami::LineFitEntry;
using namespace Ami::Qt;

QLineFitEntry::QLineFitEntry(LineFit::Method m,
			     QwtPlot* frame,
			     const QColor& c) :
  _name (LineFit::method_str(m)),
  _fit  (LineFitEntry::instance(m)),
  _curve(QString(_name.c_str()))
{
  _curve.attach(frame);
  _curve.setStyle(QwtPlotCurve::Lines);
  _curve.setPen  (QPen(c));
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

  const QChar delta(0x394);
  const QChar subzero(0x2080);
  _curve.setTitle( QString("%1[%2y/%3x=%4,y%5=%6]")
		   .arg(_name.c_str())
		   .arg(delta).arg(delta).arg(slope)
		   .arg(subzero).arg(intercept) );
}

