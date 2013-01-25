#include "DescBinning.hh"
#include "ami/qt/QtPersistent.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QStackedWidget>

using namespace Ami::Qt;

DescBinning::DescBinning(const char* name) :
  QWidget(0), 
  _bins(new QLineEdit("100")),
  _lo  (new QLineEdit("0")),
  _hi  (new QLineEdit("0")),
  _xsigma(new QLineEdit("3")),
  _xrange(new QLineEdit("0")),
  _nsigma(new QLineEdit("100")),
  _nrange(new QLineEdit("100")),
  _method(new QComboBox)
{
  _bins  ->setMaximumWidth(40);
  _lo    ->setMaximumWidth(40);
  _hi    ->setMaximumWidth(40);
  _xsigma->setMaximumWidth(40);
  _xrange->setMaximumWidth(40);
  _nsigma->setMaximumWidth(40);
  _nrange->setMaximumWidth(40);

  QStackedWidget* range = new QStackedWidget;
  
  QWidget* fixed = new QWidget;
  { QHBoxLayout* l = new QHBoxLayout;
    l->addStretch();
    l->addWidget(new QLabel("lo"));
    l->addWidget(_lo);
    l->addStretch();
    l->addWidget(new QLabel("hi"));
    l->addWidget(_hi);
    fixed->setLayout(l); }
  range->insertWidget(Fixed, fixed);
  _method->insertItem(Fixed, "Fixed");

  QWidget* auto1 = new QWidget;
  { QVBoxLayout* vl = new QVBoxLayout;
    { QHBoxLayout* l = new QHBoxLayout;
      l->addStretch();
      l->addWidget(new QLabel(QString("mean %1").arg(QChar(0x00b1))));
      l->addWidget(_xsigma);
      l->addWidget(new QLabel(QString(QChar(0x03c3))));
      l->addStretch();
      vl->addLayout(l); }
    { QHBoxLayout* l = new QHBoxLayout;
      l->addStretch();
      l->addWidget(new QLabel("from"));
      l->addWidget(_nsigma);
      l->addWidget(new QLabel("entries"));
      l->addStretch();
      vl->addLayout(l); }
    auto1->setLayout(vl); }
  range->insertWidget(Auto1, auto1);
  _method->insertItem(Auto1, "Auto1");

  QWidget* auto2 = new QWidget;
  { QVBoxLayout* vl = new QVBoxLayout;
    { QHBoxLayout* l = new QHBoxLayout;
      l->addStretch();
      l->addWidget(new QLabel(QString("[min..max] %1").arg(QChar(0x00b1))));
      l->addWidget(_xrange);
      l->addWidget(new QLabel("%"));
      l->addStretch();
      vl->addLayout(l); }
    { QHBoxLayout* l = new QHBoxLayout;
      l->addStretch();
      l->addWidget(new QLabel("from"));
      l->addWidget(_nrange);
      l->addWidget(new QLabel("entries"));
      l->addStretch();
      vl->addLayout(l); }
    auto2->setLayout(vl); }
  range->insertWidget(Auto2, auto2);
  _method->insertItem(Auto2, "Auto2");

  QStackedWidget* samples = new QStackedWidget;
  samples->insertWidget(Fixed, new QWidget);
  samples->insertWidget(Auto1, new QWidget);
  samples->insertWidget(Auto2, new QWidget);

  new QIntValidator   (1,100000,_bins);
  new QDoubleValidator(_lo);
  new QDoubleValidator(_hi);
  new QDoubleValidator(_xsigma);
  new QDoubleValidator(_xrange);
  new QIntValidator   (1,100000,_nsigma);
  new QIntValidator   (1,100000,_nrange);

  QVBoxLayout* vl = new QVBoxLayout;
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(new QLabel(name));
    layout->addWidget(_bins);
    layout->addStretch();
    layout->addWidget(_method);
    layout->addStretch();
    vl->addLayout(layout); }
  vl->addWidget(range);
  setLayout(vl);
  validate();

  connect(_lo, SIGNAL(editingFinished()), this, SLOT(validate()));
  connect(_hi, SIGNAL(editingFinished()), this, SLOT(validate()));
  connect(_method, SIGNAL(currentIndexChanged(int)), range, SLOT(setCurrentIndex(int)));
}

DescBinning::Binning DescBinning::method() const { return Binning(_method->currentIndex()); }
unsigned DescBinning::bins() const { return _bins->text().toInt(); }
double   DescBinning::lo  () const { return _lo->text().toDouble(); }
double   DescBinning::hi  () const { return _hi->text().toDouble(); }
double   DescBinning::sigma () const { return _xsigma->text().toDouble(); }
double   DescBinning::extent() const { return _xrange->text().toDouble()*0.01; }
unsigned DescBinning::nsamples() const { return (method()==Auto1 ? _nsigma : _nrange)->text().toInt(); }

void DescBinning::method(Binning b) { _method->setCurrentIndex((int)b); }
void DescBinning::bins(unsigned b) { _bins->setText(QString::number(b)); }
void DescBinning::lo  (double   v) { _lo  ->setText(QString::number(v)); }
void DescBinning::hi  (double   v) { _hi  ->setText(QString::number(v)); }
void DescBinning::sigma (double   v) { _xsigma->setText(QString::number(v)); }
void DescBinning::extent(double   v) { _xrange->setText(QString::number(v)); }
void DescBinning::nsamples(unsigned v) { _nsigma->setText(QString::number(v)); _nrange->setText(QString::number(v)); }
void DescBinning::validate()
{
  QPalette p(palette());
  if (lo() < hi())
    p.setColor(QPalette::Text, QColor(0,0,0));
  else
    p.setColor(QPalette::Text, QColor(0xc0,0,0));

  _lo->setPalette(p);
  _hi->setPalette(p);
}

void DescBinning::save(char*& p) const
{
  XML_insert( p, "QString", "_bins",
              QtPersistent::insert(p,_bins->text()) );
  XML_insert( p, "QString", "_lo",
              QtPersistent::insert(p,_lo  ->text()) );
  XML_insert( p, "QString", "_hi",
              QtPersistent::insert(p,_hi  ->text()) );
  XML_insert( p, "QString", "_xsigma",
              QtPersistent::insert(p,_xsigma->text()) );
  XML_insert( p, "QString", "_xrange",
              QtPersistent::insert(p,_xrange->text()) );
  XML_insert( p, "QString", "_nsigma",
              QtPersistent::insert(p,_nsigma->text()) );
  XML_insert( p, "QString", "_nrange",
              QtPersistent::insert(p,_nrange->text()) );
  XML_insert( p, "int", "_method",
              QtPersistent::insert(p,_method->currentIndex()) );
}

void DescBinning::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "_bins")
      _bins->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_lo")
      _lo  ->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_hi")
      _hi  ->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_xsigma")
      _xsigma->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_xrange")
      _xrange->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_nsigma")
      _nsigma->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_nrange")
      _nrange->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_method")
      _method->setCurrentIndex(QtPersistent::extract_i(p));
  XML_iterate_close(DescBinning,tag);
  validate();
}
