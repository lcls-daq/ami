#include "DescTH1F.hh"
#include "ami/qt/QtPersistent.hh"

#include <QtGui/QRadioButton>
#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>

using namespace Ami::Qt;

DescTH1F::DescTH1F(const char* name) : 
  QWidget(0), _button(new QRadioButton(name)), 
  _bins(new QLineEdit("100")), _lo(new QLineEdit("0")), _hi(new QLineEdit("1")) 
{
  _bins->setMaximumWidth(60);
  _lo  ->setMaximumWidth(60);
  _hi  ->setMaximumWidth(60);
  new QIntValidator   (_bins);
  new QDoubleValidator(_lo);
  new QDoubleValidator(_hi);
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_button);
  layout->addStretch();
  layout->addWidget(new QLabel("bins"));
  layout->addWidget(_bins);
  layout->addWidget(new QLabel("lo"));
  layout->addWidget(_lo);
  layout->addWidget(new QLabel("hi"));
  layout->addWidget(_hi);
  setLayout(layout);
}

QRadioButton* DescTH1F::button() { return _button; }
unsigned DescTH1F::bins() const { return _bins->text().toInt(); }
double   DescTH1F::lo  () const { return _lo->text().toDouble(); }
double   DescTH1F::hi  () const { return _hi->text().toDouble(); }

void DescTH1F::bins(unsigned b) { _bins->setText(QString::number(b)); }
void DescTH1F::lo  (double   v) { _lo  ->setText(QString::number(v)); }
void DescTH1F::hi  (double   v) { _hi  ->setText(QString::number(v)); }

void DescTH1F::save(char*& p) const
{
  QtPersistent::insert(p,_bins->text());
  QtPersistent::insert(p,_lo  ->text());
  QtPersistent::insert(p,_hi  ->text());
}

void DescTH1F::load(const char*& p)
{
  _bins->setText(QtPersistent::extract_s(p));
  _lo  ->setText(QtPersistent::extract_s(p));
  _hi  ->setText(QtPersistent::extract_s(p));
}
