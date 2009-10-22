#include "ImageScale.hh"

#include "ami/qt/ImageColorControl.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QPixmap>
#include <QtCore/QString>

using namespace Ami::Qt;

ImageScale::ImageScale(const QString& title,
		       const ImageColorControl& color) :
  _input (new QSpinBox),
  _canvas(new QLabel),
  _color (color),
  _pixmap(new QPixmap(1,1))
{
  _input->setRange(0,0xffff);
  _canvas->setScaledContents(true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel(title));
  layout->addWidget(_input);
  layout->addWidget(_canvas);
  setLayout(layout);

  connect(_input, SIGNAL(valueChanged(int)), this, SLOT(value_change(int)));
  connect(&color, SIGNAL(windowChanged())  , this, SLOT(scale_change()));

  _input->setValue(0);
  value_change(0);
}

ImageScale::~ImageScale() 
{
}

unsigned ImageScale::value() const 
{
  return _input->value();
}

void ImageScale::value(unsigned v)
{
  _input->setValue(v);
}

void ImageScale::value_change(int v)
{
  v /= int(_color.scale());
  if (v > 0xff) v = 0xff;
  _pixmap->fill(_color.color_table()[v]);
  _canvas->setPixmap(*_pixmap);
}

void ImageScale::scale_change()
{
  value_change(_input->value());
}
