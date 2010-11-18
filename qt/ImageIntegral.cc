#include "ami/qt/ImageIntegral.hh"
#include "ami/data/BinMath.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGroupBox>

using namespace Ami::Qt;

ImageIntegral::ImageIntegral(QWidget* p) : 
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

ImageIntegral::~ImageIntegral() {}

void ImageIntegral::update_range(int x1, int y1,
                                 int x2, int y2)
{
  QString expr = QString("[%1]%2[%3][%4]%5[%6]").
    arg(x1).
    arg(BinMath::integrate()).
    arg(x2).
    arg(y1).
    arg(BinMath::integrate()).
    arg(y2);
  _expr_edit->setText(expr);
}

const char* ImageIntegral::expression() const
{
  return qPrintable(_expr_edit->text());
}
