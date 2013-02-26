#include "ami/qt/ImageFunctions.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/qt/QtPersistent.hh"

#include "ami/data/BinMath.hh"

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>

#include <math.h>

using namespace Ami::Qt;

enum { Intg, Mean, Vari, Cont, XCoM, YCoM };

ImageFunctions::ImageFunctions(QWidget* p) : 
  QWidget(p),
  _functions    (new QComboBox),
  _expr_edit    (new QLineEdit)
{
  QStringList functions;
  functions << "Integral"
            << "Mean"
            << "Variance"
            << "Contrast"
            << "X-Center of Mass"
            << "Y-Center of Mass";
  _functions->addItems(functions);

  _expr_edit->setMinimumWidth(140);

  QVBoxLayout* l = new QVBoxLayout;
  { QHBoxLayout* h = new QHBoxLayout;
    h->addStretch();
    h->addWidget(new QLabel("Function"));
    h->addWidget(_functions);
    h->addStretch();
    l->addLayout(h); }
  l->addWidget(_plot_desc = new ScalarPlotDesc(0));
  { QGroupBox* box = new QGroupBox;
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addStretch();
    hl->addWidget(new QLabel("Range Expression"));
    hl->addStretch();
    hl->addWidget(_expr_edit);
    hl->addStretch();
    box->setLayout(hl);
    l->addWidget(box); }
  setLayout(l);

  connect(_functions, SIGNAL(currentIndexChanged(int)), this, SLOT(update_function(int)));
}

ImageFunctions::~ImageFunctions() {}

void ImageFunctions::save(char*& p) const
{
  XML_insert(p, "QComboBox", "_functions", QtPersistent::insert(p, _functions->currentIndex()) );
  XML_insert(p, "ScalarPlotDesc", "_plot_desc", _plot_desc->save(p) );
}

void ImageFunctions::load(const char*& p)
{
  XML_iterate_open(p, tag)
    if (tag.name == "_functions")
      _functions->setCurrentIndex(QtPersistent::extract_i(p));
    else if(tag.name == "_plot_desc")
      _plot_desc->load(p);
  XML_iterate_close(ImageFunctions,tag);
}

static QChar qFunction(int i) 
{
  QChar q;
  switch(i) {
  case Mean: q = Ami::BinMath::mean     (); break;
  case Vari: q = Ami::BinMath::variance (); break;
  case Cont: q = Ami::BinMath::contrast (); break;
  case XCoM: q = Ami::BinMath::xmoment  (); break;
  case YCoM: q = Ami::BinMath::ymoment  (); break;
  case Intg: 
  default: q = Ami::BinMath::integrate(); break;
  };
  return q;
}

void ImageFunctions::update_range(int x1, int y1,
                                 int x2, int y2)
{
  QChar q(qFunction(_functions->currentIndex()));
  QString expr = QString("[%1]%2[%3][%4]%5[%6]").
    arg(x1).
    arg(q).
    arg(x2).
    arg(y1).
    arg(q).
    arg(y2);
  _expr_edit->setText(expr);
}

void ImageFunctions::update_range(double xc, double yc,
                                  double r1, double r2,
                                  double f0, double f1)
{
  QChar q(qFunction(_functions->currentIndex()));
  while (!(f0<f1)) f1 += 2*M_PI;
  QString expr = QString("[%1]%2[%3][%4]%5[%6][%7]%8[%9]").
    arg(xc*BinMath::floatPrecision()).
    arg(q).
    arg(yc*BinMath::floatPrecision()).
    arg(r1*BinMath::floatPrecision()).
    arg(q).
    arg(r2*BinMath::floatPrecision()).
    arg(f0*BinMath::floatPrecision()).
    arg(q).
    arg(f1*BinMath::floatPrecision());
  _expr_edit->setText(expr);
}

void ImageFunctions::update_function(int index)
{
  QChar q(qFunction(index));
  QString t = _expr_edit->text();
  t.replace(BinMath::integrate(),q);
  t.replace(BinMath::mean     (),q);
  t.replace(BinMath::variance (),q);
  t.replace(BinMath::contrast (),q);
  t.replace(BinMath::xmoment  (),q);
  t.replace(BinMath::ymoment  (),q);
  _expr_edit->setText(t);
}

const char* ImageFunctions::expression() const
{ return _plot_desc->expr(_expr_edit->text()); }
