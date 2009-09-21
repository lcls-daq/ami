#include "ImageClient.hh"

#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageXYProjection.hh"
#include "ami/qt/ImageRPhiProjection.hh"

#include <QtGui/QPushButton>
#include <QtCore/QChar>

using namespace Ami::Qt;

ImageClient::ImageClient(const Pds::DetInfo& info, unsigned ch) :
  Client  (info,ch,new ImageDisplay)
{
  ImageDisplay& wd = static_cast<ImageDisplay&>(display());

  { QPushButton* rectB = new QPushButton("X / Y Selection");
    rectB->setCheckable(true);
    addWidget(rectB);
    _xyproj = new ImageXYProjection(_channels,NCHANNELS,*wd.plot());
    connect(rectB, SIGNAL(clicked(bool)), _xyproj, SLOT(setVisible(bool))); }

  { QPushButton* cylB = new QPushButton(QString("%1 / %2 Selection").arg(QChar(0x03c1)).arg(QChar(0x03c6)));
    cylB ->setCheckable(true);
    addWidget(cylB);
    _rfproj = new ImageRPhiProjection(_channels,NCHANNELS,*wd.plot());
    connect(cylB, SIGNAL(clicked(bool)), _rfproj, SLOT(setVisible(bool))); }

  connect(_xyproj, SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_rfproj, SIGNAL(changed()), this, SLOT(update_configuration()));
}

ImageClient::~ImageClient() {}

void ImageClient::_configure(char*& p, 
			     unsigned input, 
			     unsigned& output,
			     ChannelDefinition* ch[], 
			     int* signatures, 
			     unsigned nchannels)
{
   _xyproj->configure(p, input, output, ch, signatures, nchannels);
   _rfproj->configure(p, input, output, ch, signatures, nchannels);
}

void ImageClient::_setup_payload(Cds& cds)
{
  _xyproj->setup_payload(cds);
  _rfproj->setup_payload(cds);
}

void ImageClient::_update()
{
  _xyproj->update();
  _rfproj->update();
}
