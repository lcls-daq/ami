#include "ami/qt/RateCalculator.hh"

#include <QtCore/QString>

using namespace Ami::Qt;

RateCalculator::RateCalculator() : QLabel("0"), _entry(0), _last(0)
{
  connect(this, SIGNAL(changed(QString)), this, SLOT(change(QString)));
}

RateCalculator::~RateCalculator() {}

bool RateCalculator::set_entry(Ami::Entry* entry) {
  _entries = _last = 0;
  return (_entry   = static_cast<Ami::EntryScalar*>(entry));
}

void RateCalculator::update() {
  if (_entry && _entry->valid()) {
    _last    = _entry->entries() - _entries;
    _entries = _entry->entries();
    emit changed(QString::number(_last,'f', 0));
  } else {
    emit changed(QString("."));
  }
}

void RateCalculator::change(QString text)
{
  setText(text);
}
