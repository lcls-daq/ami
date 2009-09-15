#include "TransformConstant.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>

using namespace Ami::Qt;


TransformConstant::TransformConstant(const QString& name, double value) :
  QWidget(0),
  _name  (name), 
  _value (value) 
{
  QPushButton* remB = new QPushButton("Remove");
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel (QString("%1 = %2").arg(name).arg(value)));
  layout->addStretch();
  layout->addWidget(remB);
  setLayout(layout);
  
  connect(remB, SIGNAL(clicked()), this, SLOT(remove()));
}

TransformConstant::~TransformConstant() {}

const QString& TransformConstant::name () const { return _name; }

double         TransformConstant::value() const { return _value; }

void TransformConstant::remove() { emit removed(_name); }

