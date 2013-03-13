#include "RectangleCursors.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/QtPersistent.hh"
#include "ami/qt/Rect.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QPushButton>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using namespace Ami::Qt;

static double _limit(double i, double lo, double hi)
{
  if (i<lo) return lo;
  else if (i>hi) return hi;
  else return i;
}

RectangleCursors::RectangleCursors(ImageFrame& f,
                                   QtPWidget*  fParent) :
  QWidget(0),
  _frame(f),
  _frameParent(fParent),
  _x0(0),
  _y0(0),
  _x1(-1UL),
  _y1(-1UL),
  _edit_x0   (new QLineEdit),
  _edit_y0   (new QLineEdit),
  _edit_x1   (new QLineEdit),
  _edit_y1   (new QLineEdit),
  _xmax      (-1),
  _ymax      (-1),
  _delta_x   (new QLabel),
  _delta_y   (new QLabel),
  _npixels   (new QLabel)
{
  _edit_x0->setMaximumWidth(44);
  _edit_y0->setMaximumWidth(44);
  _edit_x1->setMaximumWidth(44);
  _edit_y1->setMaximumWidth(44);

  new QDoubleValidator(_edit_x0);
  new QDoubleValidator(_edit_y0);
  new QDoubleValidator(_edit_x1);
  new QDoubleValidator(_edit_y1);

  QPushButton* grab = new QPushButton("Grab");

  QGridLayout* layout = new QGridLayout;
  int row = 0;
  layout->addWidget(new QLabel("column"),  row, 1, ::Qt::AlignHCenter);
  layout->addWidget(new QLabel("row")   ,  row, 2, ::Qt::AlignHCenter);
  row++;
  layout->addWidget(new QLabel("top-left"),row,0);
  layout->addWidget(_edit_x0              ,row,1);
  layout->addWidget(_edit_y0              ,row,2);
  row++;
  layout->addWidget(new QLabel("btm-rght"),row,0);
  layout->addWidget(_edit_x1              ,row,1);
  layout->addWidget(_edit_y1              ,row,2);
  
  layout->addWidget(grab                ,1,3,2,1);

  row++;
  layout->addWidget(new QLabel(QString(QChar(0x0394))),row,0);
  layout->addWidget(_delta_x              ,row,1);
  layout->addWidget(_delta_y              ,row,2);
  layout->addWidget(_npixels              ,row,3);
  
  setLayout(layout);

  connect(grab, SIGNAL(clicked()), this, SLOT(grab()));

  connect(_edit_x0   , SIGNAL(editingFinished()), this, SIGNAL(edited()));
  connect(_edit_y0   , SIGNAL(editingFinished()), this, SIGNAL(edited()));
  connect(_edit_x1   , SIGNAL(editingFinished()), this, SIGNAL(edited()));
  connect(_edit_y1   , SIGNAL(editingFinished()), this, SIGNAL(edited()));
  connect( this      , SIGNAL(edited())         , this, SLOT(update_edits()));
  _set_edits();
}

RectangleCursors::~RectangleCursors()
{
}

void RectangleCursors::save(char*& p) const
{
  XML_insert(p, "int", "_x0", QtPersistent::insert(p,_x0) );
  XML_insert(p, "int", "_y0", QtPersistent::insert(p,_y0) );
  XML_insert(p, "int", "_x1", QtPersistent::insert(p,_x1) );
  XML_insert(p, "int", "_y1", QtPersistent::insert(p,_y1) );
}

void RectangleCursors::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_x0")
      _x0 = QtPersistent::extract_i(p);
    else if (tag.name == "_y0")
      _y0 = QtPersistent::extract_i(p);
    else if (tag.name == "_x1")
      _x1 = QtPersistent::extract_i(p);
    else if (tag.name == "_y1")
      _y1 = QtPersistent::extract_i(p);
  XML_iterate_close(RectangleCursors,tag);

  _set_edits();
}

void RectangleCursors::save(Rect& r) const
{
  r.x0 = unsigned(xlo());
  r.y0 = unsigned(ylo());
  r.x1 = unsigned(xhi());
  r.y1 = unsigned(yhi());
}

void RectangleCursors::load(const Rect& r)
{
  _x0 = double(r.x0);
  _y0 = double(r.y0);
  _x1 = double(r.x1);
  _y1 = double(r.y1);
  _set_edits();
  update_edits();
}

void RectangleCursors::grab() 
{
  _frame.set_cursor_input(this); 
  if (_frameParent)
    _frameParent->front();
}

void RectangleCursors::update_edits() 
{
  _x0 = _edit_x0   ->text().toDouble();
  _y0 = _edit_y0   ->text().toDouble();
  _x1 = _edit_x1   ->text().toDouble();
  _y1 = _edit_y1   ->text().toDouble();

  _x0 = _limit(_x0,0,_xmax);
  _y0 = _limit(_y0,0,_ymax);
  _x1 = _limit(_x1,0,_xmax);
  _y1 = _limit(_y1,0,_ymax);

  _set_edits();

  emit changed();
}

void RectangleCursors::_set_edits()
{
  _edit_x0   ->setText(QString::number(_x0));
  _edit_y0   ->setText(QString::number(_y0));
  _edit_x1   ->setText(QString::number(_x1));
  _edit_y1   ->setText(QString::number(_y1));
  _delta_x   ->setText(QString::number(_x1-_x0));
  _delta_y   ->setText(QString::number(_y1-_y0));
  _npixels   ->setText(QString("%1 pixels").arg((_x1-_x0+1)*(_y1-_y0+1)));
}

void RectangleCursors::draw(QImage& image)
{
  const unsigned char c = 0xff;
  const QSize& sz = image.size();

  const AxisInfo& xinfo = *_frame.xinfo();
  const AxisInfo& yinfo = *_frame.yinfo();
  { unsigned xmax = (unsigned) (xinfo.position(sz.width ())-1);
    unsigned ymax = (unsigned) (yinfo.position(sz.height())-1);
    if (_xmax != xmax || 
        _ymax != ymax) {
      _xmax = xmax;
      _ymax = ymax;
      update_edits();
    }
  }

  unsigned jlo = unsigned(xinfo.tick(xlo())), jhi = unsigned(xinfo.tick(xhi()));
  unsigned klo = unsigned(yinfo.tick(ylo())), khi = unsigned(yinfo.tick(yhi()));

//   if (jhi > _xmax) jhi=_xmax;
//   if (khi > _ymax) khi=_ymax;

  { unsigned char* cc0 = image.scanLine(klo) + jlo;
    unsigned char* cc1 = image.scanLine(khi) + jlo;
    for(unsigned j=jlo; j<=jhi; j++) {
      *cc0++ = c;
      *cc1++ = c;
    }
  }

  { for(unsigned k=klo; k<khi; k++) {
      *(image.scanLine(k+1)+jlo) = c;
      *(image.scanLine(k+1)+jhi) = c;
    }
  }
}

void RectangleCursors::mousePressEvent(double x,double y)
{
  _x0 = _limit(x,0,_xmax);
  _y0 = _limit(y,0,_ymax);
}

void RectangleCursors::mouseMoveEvent (double x,double y)
{
  _x1 = _limit(x,0,_xmax);
  _y1 = _limit(y,0,_ymax);

  _set_edits();
  emit changed();
}

void RectangleCursors::mouseReleaseEvent(double x,double y) 
{
  _frame.set_cursor_input(0);
  mouseMoveEvent(x,y);
  emit done();
}

double RectangleCursors::xlo() const { return _x0 < _x1 ? _x0 : _x1; }
double RectangleCursors::ylo() const { return _y0 < _y1 ? _y0 : _y1; }
double RectangleCursors::xhi() const { return _x0 < _x1 ? _x1 : _x0; }
double RectangleCursors::yhi() const { return _y0 < _y1 ? _y1 : _y0; }

