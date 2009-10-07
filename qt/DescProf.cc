#include "DescProf.hh"
#include "ami/qt/QtPersistent.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>

using namespace Ami::Qt;

DescProf::DescProf(const char* name) : 
  QWidget(0), _button(new QRadioButton(name)), 
  _bins(new QLineEdit("100")), _lo(new QLineEdit("0")), _hi(new QLineEdit("1")) 
{
  _bins->setMaximumWidth(60);
  _lo  ->setMaximumWidth(60);
  _hi  ->setMaximumWidth(60);
  new QIntValidator   (_bins);
  new QDoubleValidator(_lo);
  new QDoubleValidator(_hi);
  QLineEdit* var = new QLineEdit;
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_button);
  layout->addWidget(var);
  layout->addStretch();
  layout->addWidget(new QLabel("bins"));
  layout->addWidget(_bins);
  layout->addWidget(new QLabel("lo"));
  layout->addWidget(_lo);
  layout->addWidget(new QLabel("hi"));
  layout->addWidget(_hi);
  setLayout(layout);

  connect(var, SIGNAL(textEdited(const QString&)), this, SLOT(set_variable(const QString&)));
}

DescProf::DescProf(const char* name, QComboBox* box) : 
  QWidget(0), _button(new QRadioButton(name)), 
  _bins(new QLineEdit("100")), _lo(new QLineEdit("0")), _hi(new QLineEdit("1")) 
{
  _bins->setMaximumWidth(60);
  _lo  ->setMaximumWidth(60);
  _hi  ->setMaximumWidth(60);
  new QIntValidator   (_bins);
  new QDoubleValidator(_lo);
  new QDoubleValidator(_hi);
  QVBoxLayout* layout1 = new QVBoxLayout;
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(_button);
    layout->addWidget(box);
    layout->addStretch();
    layout1->addLayout(layout); }
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(new QLabel("bins"));
    layout->addWidget(_bins);
    layout->addWidget(new QLabel("lo"));
    layout->addWidget(_lo);
    layout->addWidget(new QLabel("hi"));
    layout->addWidget(_hi);
    layout1->addLayout(layout); }
  setLayout(layout1);

  connect(box, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(set_variable(const QString&)));

  set_variable(box->currentText());
}

void DescProf::set_variable(const QString& s) { _var = s; }

QRadioButton* DescProf::button() { return _button; }
const QString& DescProf::variable() const { return _var; }
unsigned DescProf::bins() const { return _bins->text().toInt(); }
double   DescProf::lo  () const { return _lo->text().toDouble(); }
double   DescProf::hi  () const { return _hi->text().toDouble(); }

void DescProf::save(char*& p) const
{
  QtPersistent::insert(p,_bins->text());
  QtPersistent::insert(p,_lo  ->text());
  QtPersistent::insert(p,_hi  ->text());
}

void DescProf::load(const char*& p)
{
  _bins->setText(QtPersistent::extract_s(p));
  _lo  ->setText(QtPersistent::extract_s(p));
  _hi  ->setText(QtPersistent::extract_s(p));
}
