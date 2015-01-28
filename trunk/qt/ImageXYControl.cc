#include "ImageXYControl.hh"

#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <math.h>

using namespace Ami::Qt;

ImageXYControl::ImageXYControl(QWidget* parent,
                               const QString&  title) :
  QGroupBox(title,parent),
  _scale (0)
{
  setAlignment(::Qt::AlignHCenter);

  QPushButton* autoB = new QPushButton("Reset");
  QPushButton* zoomB = new QPushButton("Zoom");
  QPushButton* panB  = new QPushButton("Pan");

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(zoomB);
    layout1->addWidget(autoB);
    layout1->addWidget(panB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(autoB , SIGNAL(clicked(bool)), this, SLOT(reset()));
  connect(zoomB , SIGNAL(clicked()), this, SLOT(zoom()));
  connect(panB  , SIGNAL(clicked()), this, SLOT(pan ()));
}   

ImageXYControl::~ImageXYControl()
{
}

float ImageXYControl::scale() const { return powf(2,0.5*float(_scale)); }

void   ImageXYControl::reset()
{
  _scale = 0;
  emit windowChanged();
}

void   ImageXYControl::zoom ()
{
  ++_scale;
  emit windowChanged();
}

void   ImageXYControl::pan ()
{
  --_scale;
  emit windowChanged();
}
