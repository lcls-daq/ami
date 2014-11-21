#include "ami/qt/QFit.hh"
#include "ami/qt/QFitEntry.hh"
#include "ami/qt/QLineFitEntry.hh"
#include "ami/qt/QtBase.hh"
#include "ami/data/FitEntry.hh"
#include "ami/data/LineFitEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"

#include "qwt_plot.h"

using Ami::Fit;
using Ami::FitEntry;
using namespace Ami::Qt;

static const unsigned NumberOf = Ami::Fit::NumberOf + Ami::LineFit::NumberOf;

QFit::QFit() : _frame(0), _fits(NumberOf)
{
  for(unsigned i=0; i<_fits.size(); i++)
    _fits[i] = 0;
}

QFit::~QFit() 
{
  for(unsigned i=0; i<_fits.size(); i++)
    if (_fits[i])
      delete _fits[i];
}

void QFit::attach(QwtPlot* frame) 
{
  _frame=frame; 
  for(unsigned i=0; i<_fits.size(); i++) {
    QAbsFitEntry* e = _fits[i];
    if (e) e->attach(frame);
  }
}

void QFit::show_fit(unsigned m, bool show, const QColor& c) 
{ 
  QAbsFitEntry* e = _fits[m];
  if (show) {
    if (e==0) {
      if (m < LineFit::NumberOf)
	_fits[m] = new QLineFitEntry(LineFit::Method(m), _frame, c);
      else
	_fits[m] = new QFitEntry(Fit::Function(m-LineFit::NumberOf), _frame, c);
    }
  }
  else if (e) {
    delete e;
    _fits[m]=0;
  }
}

void QFit::update_fit(const Entry& p)
{
  for(unsigned i=0; i<_fits.size(); i++) {
    QAbsFitEntry* e = _fits[i];
    if (e && _frame)
      e->fit(p);
  }
}

QFitMenu::QFitMenu(const QString& title, 
		   const QColor&  c,
		   Ami::DescEntry::Type t) :
  QMenu(title)
{
  if (t==Ami::DescEntry::Prof || t==Ami::DescEntry::Scan)
    for(unsigned i=0; i<LineFit::NumberOf; i++)
      addAction(new QFitAction(*this,
			       LineFit::method_str(LineFit::Method(i)),
			       i,c));
  if (t==Ami::DescEntry::TH1F)
    for(unsigned i=0,k=LineFit::NumberOf; i<Fit::NumberOf; i++,k++)
      addAction(new QFitAction(*this,
			       Fit::function_str(Fit::Function(i)),
			       k,c));
}

QFitMenu::~QFitMenu() {}

QFitAction::QFitAction(QFit&          host,
		       const QString& title,
		       unsigned       m,
                       const QColor&  c) :
  QAction(title,0),
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


QChEntry::QChEntry(const QtBase& o, Cds& cds, QChFitMenu& m) :
  _o(o), _m(m) { m.add(&_o, true); subscribe(cds); m.subscribe(*this); }

QChEntry::~QChEntry() { _m.unsubscribe(*this); }

void QChEntry::clear_payload()
{
  _m.add(&_o, false);
  delete this; 
}


QChFitMenu::QChFitMenu(const QString& s) : 
  QMenu(s), _type(Ami::DescEntry::Ref), _frame(0) {}

QChFitMenu::~QChFitMenu() 
{
  std::list<QChEntry*> entries(_entries);
  for(std::list<QChEntry*>::iterator it=entries.begin();
      it!=entries.end(); it++)
    delete (*it);
}

void QChFitMenu::add   (const QtBase* b, bool show)
{
  QFitMenu* m;
  QString t(b->title());
  if (show) {
    if (_fits.find(b)==_fits.end()) {
      if (_save.find(t)!=_save.end()) {
        m = _save[t];
      }
      else {
        _save[t] = m = new QFitMenu(t,b->get_color(),_type);
      }
      //  Sort the menus by name
      QList<QAction*> l = actions();
      int i=0;
      while (i<l.size()) {
	if (QString::localeAwareCompare(t,l.at(i)->text())<0) {
	  insertMenu(l.at(i),m);
	  break;
	}
	i++;
      }
      if (i==l.size())
	addMenu(m);
      m->attach(_frame);
      _fits[b] = m;
    }
  }
  else if (!show && _fits.find(b)!=_fits.end()) {
    removeAction(_fits[b]->menuAction());
    _fits.erase(b);
  }
}

void QChFitMenu::attach(QwtPlot* frame)
{
  _frame=frame;
  for(std::map<const QtBase*,QFitMenu*>::iterator it=_fits.begin();
      it!=_fits.end(); it++)
    it->second->attach(frame);
}

void QChFitMenu::update()
{
  for(std::map<const QtBase*,QFitMenu*>::iterator it=_fits.begin();
      it!=_fits.end(); it++)
    it->second->update_fit(it->first->entry());
}

void QChFitMenu::clear()
{
  for(std::map<const QtBase*,QFitMenu*>::iterator it=_fits.begin();
      it!=_fits.end(); it++)
    removeAction(it->second->menuAction());
  _fits.clear();
}

void QChFitMenu::setPlotType(Ami::DescEntry::Type t) { _type=t; }

void QChFitMenu::subscribe(QChEntry& e) { _entries.push_back(&e); }

void QChFitMenu::unsubscribe(QChEntry& e) { _entries.remove(&e); }
