#include "ImageInspect.hh"

#include "ami/qt/ImageFrame.hh"
#include "ami/qt/QtImage.hh"
#include "ami/qt/AxisInfo.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using namespace Ami::Qt;

static const int _width = 16;
static const int _height = 16;
static const int _factor = 8;

ImageInspect::ImageInspect(ImageFrame&              f) :
  QWidget(0),
  _canvas(new QLabel),
  _frame (f),
  _x0(0),
  _y0(0)
{
  setWindowTitle("Inspect");

  setAttribute(::Qt::WA_DeleteOnClose, false);

  _canvas->setAlignment(::Qt::AlignLeft | ::Qt::AlignTop);
  _canvas->setMinimumSize(_width*_factor,_height*_factor);

  QGridLayout* layout = new QGridLayout;
  layout->addWidget(_canvas,0,0);
  setLayout(layout);
}

ImageInspect::~ImageInspect()
{
}

void ImageInspect::toggle() 
{
  _frame.track_cursor_input(this); 
  show();
  raise();
}

void ImageInspect::draw(QImage& image)
{
  if (isVisible()) {
    _canvas->setPixmap(QPixmap::fromImage(image.copy(_x0,_y0,_width,_height)).scaled(_canvas->size()));
  }
}

void ImageInspect::mousePressEvent(double x,double y)
{
}

void ImageInspect::mouseMoveEvent (double x,double y)
{
  _x0 = _frame.xinfo()->tick(x);
  _y0 = _frame.yinfo()->tick(y);
  _x0 -= _width/2;
  _y0 -= _height/2;

  emit changed();
}

void ImageInspect::mouseReleaseEvent(double x,double y) 
{
  _frame.set_cursor_input(0);
  mouseMoveEvent(x,y);
}
