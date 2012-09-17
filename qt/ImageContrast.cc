#include "ami/qt/ImageContrast.hh"
#include "ami/data/BinMath.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>

#include <math.h>

using namespace Ami::Qt;

ImageContrast::ImageContrast(QWidget* p) : 
  ScalarPlotDesc(p),
  _expr_edit    (new QLineEdit)
{
  _expr_edit->setMinimumWidth(340);

  QVBoxLayout* l = static_cast<QVBoxLayout*>(layout());
  QGroupBox* box = new QGroupBox("Range Expression");
  QHBoxLayout* hl = new QHBoxLayout;
  hl->addStretch();
  hl->addWidget(_expr_edit);
  hl->addStretch();
  box->setLayout(hl);
  l->addWidget(box);
}

ImageContrast::~ImageContrast() {}

void ImageContrast::update_range(int x1, int y1,
                                 int x2, int y2)
{
  QString expr = QString("[%1]%2[%3][%4]%5[%6]").
    arg(x1).
    arg(BinMath::contrast()).
    arg(x2).
    arg(y1).
    arg(BinMath::contrast()).
    arg(y2);
  _expr_edit->setText(expr);
}

void ImageContrast::update_range(double xc, double yc,
                                 double r1, double r2,
                                 double f0, double f1)
{
  while (!(f0<f1)) f1 += 2*M_PI;
  QString expr = QString("[%1]%2[%3][%4]%5[%6][%7]%8[%9]").
    arg(xc*BinMath::floatPrecision()).
    arg(BinMath::contrast()).
    arg(yc*BinMath::floatPrecision()).
    arg(r1*BinMath::floatPrecision()).
    arg(BinMath::contrast()).
    arg(r2*BinMath::floatPrecision()).
    arg(f0*BinMath::floatPrecision()).
    arg(BinMath::contrast()).
    arg(f1*BinMath::floatPrecision());
  _expr_edit->setText(expr);
}


const char* ImageContrast::expression() const
{ return expr(_expr_edit->text()); }
