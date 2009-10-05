#include "RectangleCursors.hh"

#include "ami/qt/ImageFrame.hh"
#include "ami/qt/QtPersistent.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QPushButton>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using namespace Ami::Qt;

RectangleCursors::RectangleCursors(ImageFrame& f) :
  QWidget(0),
  Cursors(f),
  _x0(f.size().width()/4),
  _y0(f.size().width()/4),
  _x1(f.size().width()*3/4),
  _y1(f.size().width()*3/4),
  _edit_x0   (new QLineEdit),
  _edit_y0   (new QLineEdit),
  _edit_x1   (new QLineEdit),
  _edit_y1   (new QLineEdit),
  _xmax      (-1),
  _ymax      (-1)
{
  new QDoubleValidator(_edit_x0);
  new QDoubleValidator(_edit_y0);
  new QDoubleValidator(_edit_x1);
  new QDoubleValidator(_edit_y1);

  QPushButton* grab_zero = new QPushButton("Grab");
  QPushButton* grab_one  = new QPushButton("Grab");

  QGridLayout* layout = new QGridLayout;
  layout->addWidget(new QLabel("c0")    ,0,0);
  layout->addWidget(_edit_x0            ,0,1);
  layout->addWidget(_edit_y0            ,0,2);
  layout->addWidget(grab_zero           ,0,3);

  layout->addWidget(new QLabel("c1")    ,1,0);
  layout->addWidget(_edit_x1            ,1,1);
  layout->addWidget(_edit_y1            ,1,2);
  layout->addWidget(grab_one            ,1,3);

  setLayout(layout);

  connect(grab_zero, SIGNAL(clicked()), this, SLOT(grab_zero()));
  connect(grab_one , SIGNAL(clicked()), this, SLOT(grab_one ()));

  connect(_edit_x0   , SIGNAL(editingFinished()), this, SLOT(update_edits()));
  connect(_edit_y0   , SIGNAL(editingFinished()), this, SLOT(update_edits()));
  connect(_edit_x1   , SIGNAL(editingFinished()), this, SLOT(update_edits()));
  connect(_edit_y1   , SIGNAL(editingFinished()), this, SLOT(update_edits()));

  _set_edits();
}

RectangleCursors::~RectangleCursors()
{
}

void RectangleCursors::save(char*& p) const
{
  QtPersistent::insert(p,_x0);
  QtPersistent::insert(p,_y0);
  QtPersistent::insert(p,_x1);
  QtPersistent::insert(p,_y1);
}

void RectangleCursors::load(const char*& p)
{
  _x0 = QtPersistent::extract_i(p);
  _y0 = QtPersistent::extract_i(p);
  _x1 = QtPersistent::extract_i(p);
  _y1 = QtPersistent::extract_i(p);
}

void RectangleCursors::grab_zero() { _active = Zero; grab_cursor(); }
void RectangleCursors::grab_one () { _active = One ; grab_cursor(); }
void RectangleCursors::update_edits() 
{
  _x0 = _edit_x0   ->text().toDouble();
  _y0 = _edit_y0   ->text().toDouble();
  _x1 = _edit_x1   ->text().toDouble();
  _y1 = _edit_y1   ->text().toDouble();

  if (_x0 > _xmax) _x0 = _xmax;
  if (_y0 > _ymax) _y0 = _ymax;
  if (_x1 > _xmax) _x1 = _xmax;
  if (_y1 > _ymax) _y1 = _ymax;

  _set_edits();

  emit changed();
}

void RectangleCursors::_set_edits()
{
  _edit_x0   ->setText(QString::number(_x0));
  _edit_y0   ->setText(QString::number(_y0));
  _edit_x1   ->setText(QString::number(_x1));
  _edit_y1   ->setText(QString::number(_y1));
}

void RectangleCursors::draw(QImage& image)
{
  const unsigned char c = 0xff;
  const QSize& sz = image.size();
  _xmax = sz.width()-1;
  _ymax = sz.height()-1;

  unsigned jlo = unsigned(xlo()), jhi = unsigned(xhi());
  unsigned klo = unsigned(ylo()), khi = unsigned(yhi());

  if (jhi > _xmax) jhi=_xmax;
  if (khi > _ymax) khi=_ymax;

  { unsigned char* cc0 = image.bits() + klo*sz.width() + jlo;
    unsigned char* cc1 = image.bits() + khi*sz.width() + jlo;
    for(unsigned j=jlo; j<=jhi; j++) {
      *cc0++ = c;
      *cc1++ = c;
    }
  }

  { unsigned char* cc0 = image.bits() + klo*sz.width() + jlo;
    unsigned char* cc1 = image.bits() + klo*sz.width() + jhi;
    for(unsigned k=klo; k<khi; k++) {
      *(cc0 += sz.width()) = c;
      *(cc1 += sz.width()) = c;
    }
  }
}

void RectangleCursors::_set_cursor(double x,double y)
{
  switch(_active) {
  case Zero:  _x0=x; _y0=y; break;
  case One :  _x1=x; _y1=y; break;
  default: break;
  }
  _active=None;

  if (_x0 > _xmax) _x0 = _xmax;
  if (_y0 > _ymax) _y0 = _ymax;
  if (_x1 > _xmax) _x1 = _xmax;
  if (_y1 > _ymax) _y1 = _ymax;

  _set_edits();
  emit changed();
}

double RectangleCursors::xlo() const { return _x0 < _x1 ? _x0 : _x1; }
double RectangleCursors::ylo() const { return _y0 < _y1 ? _y0 : _y1; }
double RectangleCursors::xhi() const { return _x0 < _x1 ? _x1 : _x0; }
double RectangleCursors::yhi() const { return _y0 < _y1 ? _y1 : _y0; }
