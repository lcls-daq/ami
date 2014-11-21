#include "ami/qt/QFitEntry.hh"
#include "ami/data/FitEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"

#include "qwt_plot.h"

using Ami::Fit;
using Ami::FitEntry;
using namespace Ami::Qt;

QFitEntry::QFitEntry(Fit::Function m,
                     QwtPlot* frame,
                     const QColor& c) :
  _name (Fit::function_str(m)),
  _fit  (FitEntry::instance(m)),
  _curve(QString(_name.c_str()))
{
  _curve.attach(frame);
  _curve.setStyle(QwtPlotCurve::Lines);
  _curve.setPen  (QPen(c));
  _curve.setCurveAttribute(QwtPlotCurve::Inverted,true);
}

void QFitEntry::attach(QwtPlot* f) { _curve.attach(f); }

void QFitEntry::fit(const Entry& p)
{
  switch(p.desc().type()) {
  case DescEntry::TH1F: 
    _fit->fit(static_cast<const EntryTH1F&>(p));
    break;
  case DescEntry::Prof: 
    _fit->fit(static_cast<const EntryProf&>(p));
    break;
  default:
    return;
  }

  _curve.setRawData(_fit->x().data(), _fit->y().data(), _fit->x().size());

  QString v;
  std::vector<QString> names = _fit->names();
  std::vector<double> params = _fit->params();
  for(unsigned i=0; i<params.size(); i++) {
    v += QString("%1%2=%3")
      .arg(i==0 ? '[':',')
      .arg(names[i])
      .arg(params[i]);
  }
  if (params.size()) v += QString("]");
  _curve.setTitle( QString("%1%2").arg(_name.c_str()).arg(v) );
}

