#include "ami/qt/QtTable.hh"
#include "ami/qt/QtTableDisplay.hh"
#include "ami/data/AbsEval.hh"
#include "ami/data/EntryScalar.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QPalette>

#include <map>

using namespace Ami::Qt;

QtTable::QtTable() : _entry(0), _cache(0), _eval(0)
{
  _label = new QLabel("-");
  _value = new QLabel("-");

  QPushButton* xButton = new QPushButton("x");
  xButton->setMaximumHeight(12);
  xButton->setMaximumWidth (12);

  QHBoxLayout* l = new QHBoxLayout;
  l->addWidget(xButton);
  l->addSpacing(4);
  l->addWidget(_label);
  l->addStretch(1);
  l->addWidget(_value);
  setLayout(l);

  QtTableDisplay::instance()->add   (*this);

  connect(xButton, SIGNAL(clicked()), this, SIGNAL(remove()));
  connect(this, SIGNAL(entry_changed()), this, SLOT(set_label()));
  connect(this, SIGNAL(destroyed()), QtTableDisplay::instance(), SLOT(update()));
}

QtTable::~QtTable() 
{
  _entry = 0;
  if (_cache)
    delete _cache;
  if (_eval)
    delete _eval;
}

void QtTable::entry (const EntryScalar* e)
{
  _entry = e; 
  if (e) {
    emit entry_changed();
    if (_cache)
      delete _cache;
    _cache = new EntryScalar(e->desc());
    _cache->setto(*e);

    if (_eval)
      delete _eval;
    _eval = Ami::AbsEval::lookup(e->desc().stat());
  }
}

void QtTable::set_label()
{
  _label->setText(_entry->desc().name());
  _value->setText("-");
}

void QtTable::update()
{
  if (!_entry)
    return;

  if (_entry->valid()) {
    double n = _entry->entries() - _cache->entries();
    if (n > 0) {
      double v = _eval->evaluate(*_entry, *_cache, n);
      _value->setText(QString::number(v));
      _value->setPalette(QPalette());
      _cache->setto(*_entry);
      return;
    }
    _cache->setto(*_entry);
  }
  QPalette p; p.setColor(QPalette::Text, ::Qt::red);
  _value->setPalette(p);
}

