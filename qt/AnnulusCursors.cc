#include "AnnulusCursors.hh"

#include "ami/qt/ImageFrame.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QPushButton>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using namespace Ami::Qt;

static void draw_line(double _xc, double _yc,
		      double _r0, double _r1,
		      double _f , QImage& image);
static void draw_arc (double _xc, double _yc,
		      double _f0, double _f1,
		      double _r , QImage& image);


static const QChar RHO(0x03c1);
static const QChar PHI(0x03c6);
static const QChar DEG(0x00b0);
static const double DEG_TO_RAD = M_PI/180.;
static const double RAD_TO_DEG = 180./M_PI;

AnnulusCursors::AnnulusCursors(ImageFrame& f) :
  QWidget(0),
  Cursors(f),
  _xc(f.size().width()/2),
  _yc(f.size().height()/2),
  _r0(f.size().width()/4),
  _r1(f.size().width()/2),
//   _f0(-M_PI),
//   _f1( M_PI),
  _f0(0),
  _f1(0),
  _edit_xc   (new QLineEdit),
  _edit_yc   (new QLineEdit),
  _edit_inner(new QLineEdit),
  _edit_outer(new QLineEdit),
  _edit_phi0 (new QLineEdit),
  _edit_phi1 (new QLineEdit)
{
  new QDoubleValidator(_edit_xc);
  new QDoubleValidator(_edit_yc);
  new QDoubleValidator(_edit_inner);
  new QDoubleValidator(_edit_outer);
  new QDoubleValidator(_edit_phi0);
  new QDoubleValidator(_edit_phi1);

  QPushButton* grab_center = new QPushButton("Grab");
  QPushButton* grab_inner  = new QPushButton("Grab");
  QPushButton* grab_outer  = new QPushButton("Grab");
  QPushButton* grab_phi0   = new QPushButton("Grab");
  QPushButton* grab_phi1   = new QPushButton("Grab");

  QGridLayout* layout = new QGridLayout;
  layout->addWidget(new QLabel("center"),0,0);
  layout->addWidget(_edit_xc            ,0,1);
  layout->addWidget(_edit_yc            ,0,2);
  layout->addWidget(grab_center         ,0,3);

  layout->addWidget(new QLabel("inner") ,1,0);
  layout->addWidget(_edit_inner         ,1,1);
  layout->addWidget(grab_inner          ,1,2);

  layout->addWidget(new QLabel("outer") ,2,0);
  layout->addWidget(_edit_outer         ,2,1);
  layout->addWidget(grab_outer          ,2,2);

  layout->addWidget(new QLabel(QString("%1 begin [%2]").arg(PHI).arg(DEG)) ,3,0);
  layout->addWidget(_edit_phi1          ,3,1);
  layout->addWidget(grab_phi1           ,3,2);

  layout->addWidget(new QLabel(QString("%1 end [%2]").arg(PHI).arg(DEG)) ,4,0);
  layout->addWidget(_edit_phi0          ,4,1);
  layout->addWidget(grab_phi0           ,4,2);

  setLayout(layout);

  connect(grab_center, SIGNAL(clicked()), this, SLOT(grab_center()));
  connect(grab_inner , SIGNAL(clicked()), this, SLOT(grab_inner ()));
  connect(grab_outer , SIGNAL(clicked()), this, SLOT(grab_outer ()));
  connect(grab_phi0  , SIGNAL(clicked()), this, SLOT(grab_phi0  ()));
  connect(grab_phi1  , SIGNAL(clicked()), this, SLOT(grab_phi1  ()));

  connect(_edit_xc   , SIGNAL(editingFinished()), this, SLOT(update_edits()));
  connect(_edit_yc   , SIGNAL(editingFinished()), this, SLOT(update_edits()));
  connect(_edit_inner, SIGNAL(editingFinished()), this, SLOT(update_edits()));
  connect(_edit_outer, SIGNAL(editingFinished()), this, SLOT(update_edits()));
  connect(_edit_phi0 , SIGNAL(editingFinished()), this, SLOT(update_edits()));
  connect(_edit_phi1 , SIGNAL(editingFinished()), this, SLOT(update_edits()));

  _set_edits();
}

AnnulusCursors::~AnnulusCursors()
{
}

void AnnulusCursors::grab_center() { _active = Center; grab_cursor(); }
void AnnulusCursors::grab_inner () { _active = Inner ; grab_cursor(); }
void AnnulusCursors::grab_outer () { _active = Outer ; grab_cursor(); }
void AnnulusCursors::grab_phi0  () { _active = Phi0  ; grab_cursor(); }
void AnnulusCursors::grab_phi1  () { _active = Phi1  ; grab_cursor(); }
void AnnulusCursors::update_edits() 
{
  _xc = _edit_xc   ->text().toDouble();
  _yc = _edit_yc   ->text().toDouble();
  _r0 = _edit_inner->text().toDouble();
  _r1 = _edit_outer->text().toDouble();
  _f0 = -_edit_phi0 ->text().toDouble()*DEG_TO_RAD;
  _f1 = -_edit_phi1 ->text().toDouble()*DEG_TO_RAD;
  emit changed();
}

void AnnulusCursors::_set_edits()
{
  _edit_xc   ->setText(QString::number(_xc));
  _edit_yc   ->setText(QString::number(_yc));
  _edit_inner->setText(QString::number(_r0));
  _edit_outer->setText(QString::number(_r1));
  _edit_phi0 ->setText(QString::number(-_f0*RAD_TO_DEG));
  _edit_phi1 ->setText(QString::number(-_f1*RAD_TO_DEG));
}

void AnnulusCursors::draw(QImage& image)
{
  const unsigned char c = 0xff;
  const QSize& sz = image.size();
  // draw center cross
  unsigned char* cc = image.bits() + int(_yc)*sz.width() + int(_xc);
  for(int i=-5; i<=5; i++) {
    if ((i+int(_xc))>=0 && (i+int(_xc))<sz.width())      *(cc+i) = c;
    if ((i+int(_yc))>=0 && (i+int(_yc))<sz.height())     *(cc+i*sz.width()) = c;
  }
  if (_f0 != _f1) {
    draw_line(_xc,_yc,_r0,_r1,_f0,image);    // draw clockwise angular boundary
    draw_line(_xc,_yc,_r0,_r1,_f1,image);    // draw counterclockwise angular boundary
  }
  double f1 = _f0 < _f1 ? _f1 : _f1 + 2*M_PI;
  draw_arc(_xc,_yc,_f0, f1,_r0,image);  // draw inner arc
  draw_arc(_xc,_yc,_f0, f1,_r1,image);  // draw outer arc
}

void AnnulusCursors::_set_cursor(double x,double y)
{
  switch(_active) {
  case Center:  _xc=x; _yc=y; break;
  case Inner :  _r0=sqrt(pow(x-_xc,2)+pow(y-_yc,2)); break;
  case Outer :  _r1=sqrt(pow(x-_xc,2)+pow(y-_yc,2)); break;
  case Phi0  :  _f0=atan2(y-_yc,x-_xc); break;
  case Phi1  :  _f1=atan2(y-_yc,x-_xc); break;
  default: break;
  }
  _active=None;
  _set_edits();
  emit changed();
}

static double clip_r(double r, double a, double x0, double x1)
{
  if (r*a < x0 ) r = x0/a;
  if (r*a > x1)  r = x1/a;
  return r;
}

void draw_line(double _xc, double _yc,
	       double _r0, double _r1,
	       double _f, QImage& image)
{
  const QSize& sz = image.size();
  double cosf = cos(_f);
  double tanf = tan(_f);
  double sinf = tanf*cosf;
  double cotf = 1./tanf;

  //  clip to image size
  double xmax = double(sz.width() -1) - _xc;
  double ymax = double(sz.height()-1) - _yc;
  double r0;
  r0 = clip_r (_r0, cosf, -_xc, xmax);
  r0 = clip_r ( r0, sinf, -_yc, ymax);
  double r1;
  r1 = clip_r (_r1, cosf, -_xc, xmax);
  r1 = clip_r ( r1, sinf, -_yc, ymax);
  
  //  draw
  if (fabs(_f) < 0.25*M_PI) {
    for(double x=r0*cosf; x<=r1*cosf; x++) {
      double y=x*tanf;
      *(image.bits()+unsigned(x+_xc)+unsigned(y+_yc)*sz.width()) = 0xff;
    }
  }
  else if (fabs(_f-M_PI_2) < 0.25*M_PI) {
    for(double y=r0*sinf; y<=r1*sinf; y++) {
      double x=y*cotf;
      *(image.bits()+unsigned(x+_xc)+unsigned(y+_yc)*sz.width()) = 0xff;
    }
  }
  else if (fabs(_f-M_PI) < 0.25*M_PI) {
    for(double x=r0*cosf; x>=r1*cosf; x--) {
      double y=x*tanf;
      *(image.bits()+unsigned(x+_xc)+unsigned(y+_yc)*sz.width()) = 0xff;
    }
  }
  else {
    for(double y=r0*sinf; y>=r1*sinf; y--) {
      double x=y*cotf;
      *(image.bits()+unsigned(x+_xc)+unsigned(y+_yc)*sz.width()) = 0xff;
    }
  }
}


void draw_arc (double _xc, double _yc,
	       double _f0, double _f1,
	       double _r, QImage& image)
{
  if (_r!=0) {
    const QSize& sz = image.size();
    double df = 1./_r;
    for(double f=_f0; f<=_f1; f+=df) {
      int ix = int(_r*cos(f)+_xc);
      int iy = int(_r*sin(f)+_yc);
      if (ix>=0 && ix < sz.width() &&
	  iy>=0 && iy < sz.height())
	*(image.bits()+ix+iy*sz.width()) = 0xff;
    }
  }
}

