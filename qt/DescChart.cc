#include "DescChart.hh"
#include "ami/data/QtPersistent.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>

#include <stdio.h>

using namespace Ami::Qt;

DescChart::DescChart(const char* name, bool lstat) :
  QWidget(0), 
  _button(new QRadioButton("v Time")), 
  _stat  (lstat ? new QComboBox : 0),
  _pts(new QLineEdit("100")), 
  _dpt(new QLineEdit("1"))
{
  if (_stat) {
    _stat->addItem("Mean");
    _stat->addItem("StdDev");
    _stat->setCurrentIndex(0);
  }

  _pts->setMaximumWidth(60);
  new QIntValidator   (_pts);
  _dpt->setMaximumWidth(60);
  new QIntValidator   (1,((1<<16)-1),_dpt);
  QHBoxLayout* layout = new QHBoxLayout;
//   layout->addWidget(_button);
  if (_stat) {
    layout->addWidget(_stat);
    layout->addWidget(new QLabel(name));
  }
  layout->addStretch();
  { QVBoxLayout* vl = new QVBoxLayout;
    { QHBoxLayout* l = new QHBoxLayout;
      l->addWidget(new QLabel("points"));
      l->addWidget(_pts); 
      vl->addLayout(l); }
    { QHBoxLayout* l = new QHBoxLayout;
      l->addWidget(new QLabel("prescale"));
      l->addWidget(_dpt);
      vl->addLayout(l); }
    layout->addLayout(vl); }
  _pts->setEnabled(true);
  _dpt->setEnabled(true);
  setLayout(layout);
}

QRadioButton* DescChart::button() { return _button; }

Ami::DescScalar::Stat DescChart::stat() const
{
  return Ami::DescScalar::Stat(_stat ? _stat->currentIndex():0);
}

unsigned DescChart::pts() const { return _pts->text().toInt(); }
unsigned DescChart::dpt() const { return _dpt->text().toInt(); }

void DescChart::save(char*& p) const
{
  XML_insert(p, "QLineEdit", "_pts", QtPersistent::insert(p,_pts->text()) );
  XML_insert(p, "QLineEdit", "_dpt", QtPersistent::insert(p,_dpt->text()) );
}

void DescChart::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_pts")
      _pts->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_dpt")
      _dpt->setText(QtPersistent::extract_s(p));
  XML_iterate_close(DescChart,tag);
}
