#include "ImageColorControl.hh"

#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <math.h>

using namespace Ami::Qt;

enum { Mono=0, Thermal=1 };

static QVector<QRgb>* monochrome_palette()
{
  QVector<QRgb>* color_table = new QVector<QRgb>(256);
  for (int i = 0; i < 256; i++)
    color_table->insert(i, qRgb(i,i,i));
  return color_table;
}

static QVector<QRgb>* thermal_palette()
{
  QVector<QRgb>* color_table = new QVector<QRgb>(256);
  for (int i = 0; i < 43; i++)  // black - red
    color_table->insert(  0+i, qRgb(i*6,0,0));
  for (int i = 0; i < 86; i++)  // red - green
    color_table->insert( 43+i, qRgb(255-i*3,i*3,0));
  for (int i = 0; i < 86; i++)  // green - blue
    color_table->insert(129+i, qRgb(0,255-i*3,i*3));
  for (int i = 0; i < 40; i++)  // blue - violet
    color_table->insert(215+i, qRgb(i*3,0,255-i*3));
  color_table->insert(255, qRgb(255,255,255));
  return color_table;
}

ImageColorControl::ImageColorControl(QWidget* parent,
				   const QString&  title) :
  QGroupBox(title,parent),
  _scale (0),
  _scale_max(new QLabel)
{
  setAlignment(::Qt::AlignHCenter);

  QPushButton* autoB = new QPushButton("Reset");
  QPushButton* zoomB = new QPushButton("Zoom");
  QPushButton* panB  = new QPushButton("Pan");

  QImage palette(256,16,QImage::Format_Indexed8);
  { unsigned char* dst = palette.bits();
    for(unsigned k=0; k<256*16; k++) *dst++ = k&0xff; }

  QRadioButton* monoB  = new QRadioButton; 
  palette.setColorTable(*(_color_table = monochrome_palette()));
  QLabel* monoC = new QLabel;
  monoC->setPixmap(QPixmap::fromImage(palette));
  delete _color_table;

  QRadioButton* colorB = new QRadioButton; 
  palette.setColorTable(*(_color_table = thermal_palette()));
  QLabel* colorC = new QLabel;
  colorC->setPixmap(QPixmap::fromImage(palette));

  _paletteGroup = new QButtonGroup;
  _paletteGroup->addButton(monoB ,Mono);
  _paletteGroup->addButton(colorB,Thermal);

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(zoomB);
    layout1->addWidget(autoB);
    layout1->addWidget(panB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    { QGridLayout* layout2 = new QGridLayout;
      layout2->addWidget(monoB,0,0);
      layout2->addWidget(monoC,0,1,1,2);
      layout2->addWidget(colorB,1,0);
      layout2->addWidget(colorC,1,1,1,2);
      layout2->addWidget(new QLabel("0"),2,1,::Qt::AlignLeft);
      layout2->addWidget(_scale_max,2,2,::Qt::AlignRight); 
      layout1->addLayout(layout2); }
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(autoB , SIGNAL(clicked(bool)), this, SLOT(set_auto(bool)));
  connect(zoomB , SIGNAL(clicked()), this, SLOT(zoom()));
  connect(panB  , SIGNAL(clicked()), this, SLOT(pan ()));
  connect(_paletteGroup, SIGNAL(buttonClicked(int)), this, SLOT(set_palette(int)));
  connect(this  , SIGNAL(windowChanged()), this, SLOT(show_scale()));

  colorB->setChecked(true);
  show_scale();
}   

ImageColorControl::~ImageColorControl()
{
}

void ImageColorControl::save(char*& p) const
{
  QtPersistent::insert(p,_scale);
  QtPersistent::insert(p,_paletteGroup->checkedId());
}

void ImageColorControl::load(const char*& p)
{
  _scale = QtPersistent::extract_i(p);
  _paletteGroup->button(QtPersistent::extract_i(p))->setChecked(true);
}

double ImageColorControl::scale() const { return pow(2,0.5*double(-_scale)); }

const QVector<QRgb>& ImageColorControl::color_table() const { return *_color_table; }

void   ImageColorControl::set_palette(int p)
{
  delete _color_table;
  _color_table = (p==Mono) ? monochrome_palette() : thermal_palette();

  emit windowChanged();
}

void   ImageColorControl::show_scale()
{
  unsigned v = static_cast<unsigned>(0xff*scale());
  _scale_max->setText(QString::number(v));
}

void   ImageColorControl::set_auto(bool s)
{
  _scale = 0;
  emit windowChanged();
}

void   ImageColorControl::zoom ()
{
  ++_scale;
  emit windowChanged();
}

void   ImageColorControl::pan ()
{
  --_scale;
  emit windowChanged();
}

