#include "ami/qt/StatCalculator.hh"

#include <QtCore/QString>

using namespace Ami::Qt;

StatCalculator::StatCalculator() : QLabel("0"), _entry(0)
{
  connect(this, SIGNAL(changed(QString)), this, SLOT(change(QString)));
}

StatCalculator::~StatCalculator() {}

bool StatCalculator::set_entry(Ami::Entry* entry) {
  reset();
  bool result = (entry!=0);
  if (result)
    _entry = static_cast<Ami::EntryScalar*>(entry);
  else
    _entry = 0;
  return result;
}

void StatCalculator::change(QString text)
{
  setText(text);
}
