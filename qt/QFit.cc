#include "ami/qt/QFit.hh"
#include "ami/qt/QtBase.hh"
#include "ami/data/FitEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"

#include "qwt_plot.h"

using Ami::Fit;
using Ami::FitEntry;
using namespace Ami::Qt;

static QColor _color[]  = { QColor(129  , 0, 0),
			    QColor(255, 129, 0),
			    QColor(129, 129, 0) };

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

QFit::QFit() : _frame(0) 
{
  for(unsigned i=0; i<Fit::NumberOf; i++)
    _fits[Fit::Function(i)] = 0;
}

QFit::~QFit() 
{
  for(unsigned i=0; i<Fit::NumberOf; i++)
    if (_fits[Fit::Function(i)])
      delete _fits[Fit::Function(i)];
}

void QFit::attach(QwtPlot* frame) 
{
  _frame=frame; 
  for(unsigned i=0; i<Fit::NumberOf; i++) {
    QFitEntry* e = _fits[Fit::Function(i)];
    if (e) e->attach(frame);
  }
}

void QFit::show_fit(Fit::Function m, bool show, const QColor& c) 
{ 
  QFitEntry* e = _fits[m];
  if (show) {
    if (e==0)
      _fits[m] = new QFitEntry(m, _frame, c);
  }
  else if (e) {
    delete e;
    _fits[m]=0;
  }
}

void QFit::update_fit(const Entry& p)
{
  for(unsigned i=0; i<Fit::NumberOf; i++) {
    QFitEntry* e = _fits[Fit::Function(i)];
    if (e && _frame)
      e->fit(p);
  }
}

QFitMenu::QFitMenu(const QString& title) :
  QMenu(title)
{
  for(unsigned i=0; i<Fit::NumberOf; i++)
    addAction(new QFitAction(*this,Fit::Function(i),_color[i]));
}

QFitMenu::QFitMenu(const QString& title, const QColor& c) :
  QMenu(title)
{
  for(unsigned i=0; i<Fit::NumberOf; i++)
    addAction(new QFitAction(*this,Fit::Function(i),c));
}

QFitMenu::~QFitMenu() {}

QFitAction::QFitAction(QFit&       host,
                       Fit::Function m,
                       const QColor& c) :
  QAction(Fit::function_str(m),0),
  _host  (host),
  _function(m),
  _color   (c)
{
  setCheckable(true);
  connect(this, SIGNAL(changed()), this, SLOT(show_fit()));
}

QFitAction::~QFitAction() {}

void QFitAction::show_fit() 
{ 
  _host.show_fit(_function, isChecked(), _color); 
}


QChFitMenu::QChFitMenu(const QString& s) : 
  QMenu(s), _frame(0) {}

QChFitMenu::~QChFitMenu() {}

void QChFitMenu::add   (QtBase* b, bool show)
{
  QFitMenu* m;
  QString t(b->title());
  if (show) {
    if (_fits.find(b)==_fits.end()) {
      if (_save.find(t)!=_save.end()) {
        m = _save[t];
      }
      else {
        _save[t] = m = new QFitMenu(t,b->get_color());
      }
      addMenu(m);
      m->attach(_frame);
      _fits[b] = m;
    }
  }
  else if (!show && _fits.find(b)!=_fits.end()) {
    _fits.erase(b);
  }
}

void QChFitMenu::attach(QwtPlot* frame)
{
  _frame=frame;
  for(std::map<QtBase*,QFitMenu*>::iterator it=_fits.begin();
      it!=_fits.end(); it++)
    it->second->attach(frame);
}

void QChFitMenu::update()
{
  for(std::map<QtBase*,QFitMenu*>::iterator it=_fits.begin();
      it!=_fits.end(); it++)
    it->second->update_fit(it->first->entry());
}

void QChFitMenu::clear()
{
  for(std::map<QtBase*,QFitMenu*>::iterator it=_fits.begin();
      it!=_fits.end(); it++)
    removeAction(it->second->menuAction());
  _fits.clear();
}
