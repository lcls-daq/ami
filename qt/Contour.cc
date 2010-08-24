#include "ami/qt/Contour.hh"
#include "ami/qt/QtPersistent.hh"
#include "ami/qt/RectangleCursors.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QImage>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtCore/QString>

using namespace Ami::Qt;

//#define PROJ_Y

Contour::Contour(const char* x, const char* y,
		 const ImageFrame&       image,
		 const RectangleCursors& frame) :
  _image(image),
  _frame(frame),
  _discrimLevel(0.0)
{
  setup(x,y);
  show();
}

Contour::~Contour()
{
}

void Contour::draw(QImage& image)
{
#ifdef PROJ_Y
  //
  //  Calculate y extremes of the contour
  //
  Ami::Contour f = value();
  double x = _frame.xlo();
  double ymin=f.value(x), ymax=ymin;
  while (++x<=_frame.xhi()) {
    double y = f.value(x);
    if (y < ymin) { ymin = y; }
    if (y > ymax) { ymax = y; }
  }

  if (ymax - ymin > _frame.yhi() - _frame.ylo())  // ROI (_frame) can't contain the contour
    return;

  //
  //  Draw the f(X) contour and the contour boundaries of the frame
  //
  const unsigned char c = 0xff;

  const AxisInfo& xinfo = *_image.xinfo();
  const AxisInfo& yinfo = *_image.yinfo();
  unsigned w = image.size().width();
  
  for(int ix = xinfo.lo(); ix < xinfo.hi(); ix++) {
    float y = f.value(xinfo.position(ix));
    *(image.bits() + yinfo.tick(y)*w + ix) = c;
  }

  float y0 = _frame.ylo() - ymin;
  float y1 = _frame.yhi() - ymax;
  unsigned ixhi = xinfo.tick(_frame.xhi());
  for(unsigned ix = xinfo.tick(_frame.xlo()); ix <= ixhi; ix++) {
    float y = f.value(xinfo.position(ix));
    *(image.bits() + yinfo.tick(y+y0)*w + ix) = c;
    *(image.bits() + yinfo.tick(y+y1)*w + ix) = c;
  }
#else
  //
  //  Calculate x extremes of the contour
  //
  Ami::Contour f = value();
  double y = _frame.ylo();
  double xmin=f.value(y), xmax=xmin;
  while (++y<=_frame.yhi()) {
    double x = f.value(y);
    if (x < xmin) { xmin = x; }
    if (x > xmax) { xmax = x; }
  }

  if (xmax - xmin > _frame.xhi() - _frame.xlo())  // ROI (_frame) can't contain the contour
    return;

  //
  //  Draw the f(Y) contour and the contour boundaries of the frame
  //
  const unsigned char c = 0xff;

  const AxisInfo& xinfo = *_image.xinfo();
  const AxisInfo& yinfo = *_image.yinfo();
  unsigned w = image.size().width();
  
  for(int iy = yinfo.lo(); iy < yinfo.hi(); iy++) {
    float x = f.value(yinfo.position(iy));
    *(image.bits() + iy*w + xinfo.tick(x)) = c;
  }

  float x0 = _frame.xlo() - xmin;
  float x1 = _frame.xhi() - xmax;
  unsigned iyhi = yinfo.tick(_frame.yhi());
  for(unsigned iy = yinfo.tick(_frame.ylo()); iy <= iyhi; iy++) {
    float x = f.value(yinfo.position(iy));
    *(image.bits() + iy*w + xinfo.tick(x+x0)) = c;
    *(image.bits() + iy*w + xinfo.tick(x+x1)) = c;
  }
#endif
}

static unsigned _superscript[] = { 0, 0, 0x00b2, 0x00b3, 0x2074, 0x2075, 0x2076, 0x2077, 0x2078, 0x2079 };

void Contour::setup(const char* x, const char* y)
{
  QHBoxLayout* layout2 = new QHBoxLayout;
  layout2->addStretch();
  layout2->addWidget(new QLabel(QString("%1 = ").arg(y)));
  _c[0] = new QLineEdit;
  layout2->addWidget(_c[0]);
  for(unsigned i=1; i<=Ami::Contour::MaxOrder; i++) {
    layout2->addWidget(new QLabel(" + "));
    layout2->addWidget(_c[i] = new QLineEdit);
    if (i<2)
      layout2->addWidget(new QLabel(QString("%1").arg(x)));
    else
      layout2->addWidget(new QLabel(QString("%1%2").arg(x).arg(QChar(_superscript[i]))));
  }
  layout2->addWidget(new QLabel("  Discrim"));
  layout2->addWidget(_discrimLevelEdit = new QLineEdit);
  layout2->addStretch();
  setLayout(layout2);


  for(unsigned i=0; i<Ami::Contour::MaxOrder+1; i++) {
    new QDoubleValidator(_c[i]);
    _c[i]->setMaximumWidth(40);
  }
  new QDoubleValidator(_discrimLevelEdit);
  _discrimLevelEdit->setMaximumWidth(40);
  _discrimLevelEdit->setText(QString::number(0.0));
}

Ami::Contour Contour::value() const
{
  float v[Ami::Contour::MaxOrder+1];
  for(unsigned i=0; i<Ami::Contour::MaxOrder+1; i++)
    v[i] = _c[i]->text().toDouble();
  return Ami::Contour(v,Ami::Contour::MaxOrder+1,_discrimLevelEdit->text().toDouble());
}

void Contour::save(char*& p) const 
{
  for(unsigned i=0; i<Ami::Contour::MaxOrder+1; i++)
    QtPersistent::insert(p,_c[i]->text().toDouble());
  QtPersistent::insert(p,_discrimLevelEdit->text().toDouble());
}

void Contour::load(const char*& p)
{
  for(unsigned i=0; i<Ami::Contour::MaxOrder+1; i++)
    _c[i]->setText(QString::number(QtPersistent::extract_d(p)));
  _discrimLevelEdit->setText(QString::number(QtPersistent::extract_d(p)));
}
