#include "ImageFrame.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/Cursors.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageInspect.hh"
#include "ami/qt/ImageMarker.hh"
#include "ami/qt/QtImage.hh"
#include "ami/data/Entry.hh"
#include "ami/service/Semaphore.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <QtGui/QMouseEvent>

#include <QtGui/QBoxLayout>
#include <QtGui/QLabel>

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
  QWidget     (parent), 
  _engine     (*this, control),
  _canvas     (new QLabel),
  _c          (0)
{
  //  const unsigned sz = CanvasSizeDefault + CanvasSizeIncrease;
  //  _canvas->setMinimumSize(sz,sz);
  _canvas->setAlignment(::Qt::AlignLeft | ::Qt::AlignTop);

  QGridLayout* layout = new QGridLayout;
  layout->addWidget(_canvas,0,0);
  setLayout(layout);

  connect(&control, SIGNAL(windowChanged()), this , SLOT(scale_changed()));
}

ImageFrame::~ImageFrame() {}

void ImageFrame::attach(QtImage* image) 
{
  _engine.qimage(image);
  if (image) {
    image->set_color_table(_engine.control().color_table());

    if (image->scalexy()) {
      const unsigned sz = CanvasSizeDefault + CanvasSizeIncrease;
      _canvas->setMinimumSize(sz,sz);
    }

    QSize sz(_canvas->size());
    sz.rwidth()  += CanvasSizeIncrease;
    sz.rheight() += CanvasSizeIncrease;
    QGridLayout* l = static_cast<QGridLayout*>(layout()); 
    image->canvas_size(sz,*l);
  }
}

void ImageFrame::scale_changed()
{
  QtImage* img = _engine.qimage();
  if (img) img->set_color_table(_engine.control().color_table());
  replot();
}

void ImageFrame::add_marker   (ImageMarker& m) 
{
  _engine.render_sync();
  _markers.push_back(&m); 
}
void ImageFrame::remove_marker(ImageMarker& m)
{
  _engine.render_sync();
  _markers.remove(&m); 
}

ImageInspect* ImageFrame::inspector()
{
  ImageInspect* r = new ImageInspect(*this);
  add_marker(*r);
  connect(r, SIGNAL(changed()), this, SLOT(replot()));
  return r;
}

void ImageFrame::replot()
{
  if (!_canvas->isVisible())
    return;

  _engine.render();
}

void ImageFrame::render_image(QImage& output)
{
  for(std::list<ImageMarker*>::const_iterator it=_markers.begin(); it!=_markers.end(); it++) 
    (*it)->draw(output);
}

void ImageFrame::render_pixmap(QImage& output)
{
  if (_engine.qimage()->scalexy())
    _canvas->setPixmap(QPixmap::fromImage(output).scaled(_canvas->size()));
  else
    _canvas->setPixmap(QPixmap::fromImage(output));
}

void ImageFrame::mousePressEvent(QMouseEvent* e)
{
  QPoint p2 = _canvas->pos();

  unsigned ix = e->x() - p2.x();
  unsigned iy = e->y() - p2.y();
  if (_c && _engine.qimage())
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

  //  QWidget::mousePressEvent(e);
  QWidget::mouseMoveEvent(e);
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

void ImageFrame::set_cursor_input(Cursors* c) 
{
  _c=c; 
  raise();
}

void ImageFrame::track_cursor_input(Cursors* c)
{
  set_cursor_input(c);
  setMouseTracking(true);
  _canvas->setMouseTracking(true);
}

void ImageFrame::set_grid_scale(double scalex, double scaley)
{                                               
  if (_engine.qimage()) {
    _engine.qimage()->set_grid_scale(scalex,scaley);
  }
}

static AxisBins _defaultInfo(2,0,1);

const AxisInfo* ImageFrame::xinfo() const { 
  return _engine.qimage() ? _engine.qimage()->xinfo() : &_defaultInfo; 
}

const AxisInfo* ImageFrame::yinfo() const { 
  return _engine.qimage() ? _engine.qimage()->yinfo() : &_defaultInfo;
}

static Pds::ClockTime _defaultTime(0,0);

const Pds::ClockTime& ImageFrame::time() const {
  return _engine.qimage() ? _engine.qimage()->entry().time() : _defaultTime;
}

