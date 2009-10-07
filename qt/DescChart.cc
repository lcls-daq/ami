#include "DescChart.hh"
#include "ami/qt/QtPersistent.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>

using namespace Ami::Qt;

DescChart::DescChart(const char* name, double dpt) : 
  QWidget(0), _button(new QRadioButton(name)), _pts(new QLineEdit("100")), _dpt(dpt)
{
  _pts->setMaximumWidth(60);
  new QIntValidator   (_pts);
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_button);
  layout->addStretch();
  layout->addWidget(new QLabel("points"));
  layout->addWidget(_pts);
  //  layout->addWidget(new QLabel(QString("%1 seconds/pt").arg(dpt)));
  setLayout(layout);
}

QRadioButton* DescChart::button() { return _button; }
unsigned DescChart::pts() const { return _pts->text().toInt(); }
double   DescChart::dpt() const { return _dpt; }

void DescChart::save(char*& p) const
{
  QtPersistent::insert(p,_pts->text());
  QtPersistent::insert(p,_dpt);
}

void DescChart::load(const char*& p)
{
  _pts->setText(QtPersistent::extract_s(p));
  _dpt = QtPersistent::extract_d(p);
}
