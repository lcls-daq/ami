#include "ami/qt/Contour.hh"
#include "ami/qt/QtPersistent.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QImage>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtCore/QString>

using namespace Ami::Qt;

Contour::Contour() 
{
  setup("X","Y");
  show();
}

Contour::~Contour()
{
}

void Contour::draw(QImage& image)
{
  const unsigned char c = 0xff;
  const QSize& sz = image.size();
  unsigned xmax = sz.width()-1;
  unsigned ymax = sz.height()-1;

  Ami::Contour cnt = value();
  for(unsigned j=0; j<=xmax; j++) {
    float y = cnt.value(float(j));
    if (y>=0 && y<=ymax) {
      unsigned char* cc = image.bits() + int(y)*sz.width() + j;
      *cc = c;
    }
  }
}

static unsigned _superscript[] = { 0, 0, 0x00b2, 0x00b3, 0x2074, 0x2075, 0x2076, 0x2077, 0x2078, 0x2079 };

void Contour::setup(const char* x, const char* y)
{
  QHBoxLayout* layout2 = new QHBoxLayout;
  layout2->addStretch();
  layout2->addWidget(new QLabel(QString("%1 = ").arg(y)));
  layout2->addWidget(_c[0] = new QLineEdit);
  for(unsigned i=1; i<=Ami::Contour::MaxOrder; i++) {
    layout2->addWidget(new QLabel(" + "));
    layout2->addWidget(_c[i] = new QLineEdit);
    if (i<2)
      layout2->addWidget(new QLabel(QString("%1").arg(x)));
    else
      layout2->addWidget(new QLabel(QString("%1%2").arg(x).arg(QChar(_superscript[i]))));
  }
  layout2->addStretch();
  setLayout(layout2);

  for(unsigned i=0; i<Ami::Contour::MaxOrder+1; i++) {
    new QDoubleValidator(_c[i]);
    _c[i]->setMaximumWidth(40);
  }
}

Ami::Contour Contour::value() const
{
  float v[Ami::Contour::MaxOrder+1];
  for(unsigned i=0; i<Ami::Contour::MaxOrder+1; i++)
    v[i] = _c[i]->text().toDouble();
  return Ami::Contour(v,Ami::Contour::MaxOrder+1);
}

void Contour::save(char*& p) const 
{
  for(unsigned i=0; i<Ami::Contour::MaxOrder+1; i++)
    QtPersistent::insert(p,_c[i]->text().toDouble());
}

void Contour::load(const char*& p)
{
  for(unsigned i=0; i<Ami::Contour::MaxOrder+1; i++)
    _c[i]->setText(QString::number(QtPersistent::extract_d(p)));
}
