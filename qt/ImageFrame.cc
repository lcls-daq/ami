#include "ImageFrame.hh"

#include "ami/qt/Cursors.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageMarker.hh"
#include "ami/qt/QtImage.hh"

#include <QtGui/QMouseEvent>

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>
#include <QtGui/QPaintEngine>

using namespace Ami::Qt;

// static void _shift(unsigned& x,
// 		   unsigned& y,
// 		   int s)
// {
//   if (s >= 0) {
//     x <<= s;
//     y <<= s;
//   }
//   else {
//     x >>= -s;
//     y >>= -s;
//   }
// }

ImageFrame::ImageFrame(QWidget* parent,
		       const ImageColorControl& control) : 
  QWidget(parent), 
  _control(control),
  _canvas(new QLabel),
  _qimage(0),
  _c(0)
{
  unsigned sz = 512 + 4;
  _scroll = new QScrollArea;
  _scroll->setBackgroundRole(QPalette::Dark);
  _scroll->setWidget(_canvas);
  _scroll->setMinimumSize(sz,sz);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_scroll);
//   layout->addWidget(_canvas);
  layout->setSizeConstraint(QLayout::SetFixedSize);
  setLayout(layout);

  connect(&_control , SIGNAL(windowChanged()), this , SLOT(scale_changed()));
}

ImageFrame::~ImageFrame() {}

void ImageFrame::attach(QtImage& image) 
{
  _qimage = &image; 
  _qimage->set_color_table(_control.color_table());
}

void ImageFrame::scale_changed()
{
  if (_qimage) _qimage->set_color_table(_control.color_table());
  replot();
}

void ImageFrame::add_marker   (ImageMarker& m) { _markers.push_back(&m); }
void ImageFrame::remove_marker(ImageMarker& m) { _markers.remove(&m); }

void ImageFrame::replot()
{
  if (_qimage) {
    QImage& output = _qimage->image(-_control.scale());
    for(std::list<ImageMarker*>::const_iterator it=_markers.begin(); it!=_markers.end(); it++) 
      (*it)->draw(output);

    _canvas->setPixmap(QPixmap::fromImage(output));
    _canvas->resize(output.size());
  }
}

void ImageFrame::mousePressEvent(QMouseEvent* e)
{
//   QScrollBar* h = _scroll->horizontalScrollBar();
//   QScrollBar* v = _scroll->verticalScrollBar();

//   printf("ImageFrame mouse %d,%d  scroll area %d,%d  bars %d,%d\n",
// 	 e->x(),e->y(), p.x(),p.y(), h->value(),v->value());

  QPoint p1 = _scroll->pos();
  QPoint p2 = _canvas->pos();

  unsigned ix = e->x() - p1.x() - p2.x();
  unsigned iy = e->y() - p1.x() - p2.y();
  //  _shift(ix,iy,-_xyshift);
  double x(ix),y(iy);
  if (_c)
    _c->set_cursor(x,y);
  QWidget::mousePressEvent(e);
}

void ImageFrame::set_cursor_input(Cursors* c) { _c=c; }
