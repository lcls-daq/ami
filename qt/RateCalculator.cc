#include "ami/qt/RateCalculator.hh"

#include <QtCore/QString>

using namespace Ami::Qt;

RateCalculator::RateCalculator() : QLabel("0"), _entry(0)
{
  clock_gettime(CLOCK_REALTIME,&_last);
  connect(this, SIGNAL(changed(QString)), this, SLOT(change(QString)));
}

RateCalculator::~RateCalculator() {}

bool RateCalculator::set_entry(Ami::Entry* entry) {
  _entries = 0;
  bool result = (entry!=0);
  if (result)
    _entry = static_cast<Ami::EntryScalar*>(entry);
  else
    _entry = 0;
  return result;
}

void RateCalculator::update() {
  if (_entry && _entry->valid()) {
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME,&tv);
    double d = double(tv.tv_sec - _last.tv_sec)+1.e-9*(double(tv.tv_nsec)-double(_last.tv_nsec));
    double n = (_entry->entries() - _entries)/d;
    _last    = tv;
    _entries = _entry->entries();
    emit changed(QString::number(n,'f', 0));
  } else {
    emit changed(QString("."));
  }
}

void RateCalculator::change(QString text)
{
  setText(text);
}
