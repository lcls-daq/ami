#include "ImageFrame.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/Cursors.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageMarker.hh"
#include "ami/qt/QtImage.hh"

#include <QtGui/QMouseEvent>

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
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

static const int CanvasSizeDefault  = 512;
static const int CanvasSizeIncrease = 4;

ImageFrame::ImageFrame(QWidget* parent,
		       const ImageColorControl& control) : 
  QWidget(parent), 
  _control(control),
  _canvas(new QLabel),
  _qimage(0),
  _xyscale(false),
  _c(0)
{
  unsigned sz = CanvasSizeDefault + CanvasSizeIncrease;
  _canvas->setMinimumSize(sz,sz);
  _canvas->setAlignment(::Qt::AlignLeft | ::Qt::AlignTop);

  QHBoxLayout* layout = new QHBoxLayout;
  //  layout->addWidget(_scroll);
  layout->addWidget(_canvas);
  setLayout(layout);

  connect(&_control , SIGNAL(windowChanged()), this , SLOT(scale_changed()));
}

ImageFrame::~ImageFrame() {}

void ImageFrame::attach(QtImage* image) 
{
  _qimage = image; 
  if (_qimage)
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
    QImage& output = _qimage->image(_control.scale(),_control.linear());
    for(std::list<ImageMarker*>::const_iterator it=_markers.begin(); it!=_markers.end(); it++) 
      (*it)->draw(output);

    if (_xyscale)
      _canvas->setPixmap(QPixmap::fromImage(output).scaled(_canvas->size(),
							   ::Qt::KeepAspectRatio));
    else {
      QSize sz = output.size();
      sz.rwidth () += CanvasSizeIncrease;
      sz.rheight() += CanvasSizeIncrease;
      if (sz != _canvas->size())
	_canvas->setMinimumSize(sz);
      _canvas->setPixmap(QPixmap::fromImage(output));
    }
  }
}

void ImageFrame::mousePressEvent(QMouseEvent* e)
{
  QPoint p2 = _canvas->pos();

  unsigned ix = e->x() - p2.x();
  unsigned iy = e->y() - p2.y();
  if (_c && _qimage)
    _c->mousePressEvent(xinfo()->position(ix),
			yinfo()->position(iy));
  QWidget::mousePressEvent(e);
}

void ImageFrame::mouseMoveEvent(QMouseEvent* e)
{
  QPoint p2 = _canvas->pos();

  unsigned ix = e->x() - p2.x();
  unsigned iy = e->y() - p2.y();
  if (_c)
    _c->mouseMoveEvent(xinfo()->position(ix),
		       yinfo()->position(iy));

  QWidget::mousePressEvent(e);
}

void ImageFrame::mouseReleaseEvent(QMouseEvent* e)
{
  QPoint p2 = _canvas->pos();

  unsigned ix = e->x() - p2.x();
  unsigned iy = e->y() - p2.y();
  if (_c)
    _c->mouseReleaseEvent(xinfo()->position(ix),
			  yinfo()->position(iy));

  QWidget::mouseReleaseEvent(e);
}

void ImageFrame::set_cursor_input(Cursors* c) { _c=c; }

void ImageFrame::autoXYScale(bool v)
{
  _xyscale = v;
}

