#include "ami/qt/DoubleEdit.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QValidator>

using namespace Ami::Qt;

DoubleEdit::DoubleEdit(double v) :
  QLineEdit(QString::number(v)),
  _v       (v) 
{
  new QDoubleValidator(this);
  connect(this, SIGNAL(editingFinished()), this, SLOT(set_value()));
}

DoubleEdit::~DoubleEdit() {}

void DoubleEdit::value(double v)
{
  setText(QString::number(v));
  emit editingFinished();
}

void DoubleEdit::set_value()
{
  double v = text().toDouble();
  if (v != _v) {
    _v = v;
    emit changed();
  }
}
