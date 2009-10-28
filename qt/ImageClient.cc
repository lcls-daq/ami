#include "ImageClient.hh"

#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageXYProjection.hh"
#include "ami/qt/ImageRPhiProjection.hh"
#include "ami/qt/PeakFinder.hh"

#include <QtGui/QPushButton>
#include <QtCore/QChar>

using namespace Ami::Qt;

ImageClient::ImageClient(QWidget* parent,const Pds::DetInfo& info, unsigned ch) :
  Client  (parent,info,ch,new ImageDisplay)
{
  ImageDisplay& wd = static_cast<ImageDisplay&>(display());

  { QPushButton* rectB = new QPushButton("X / Y Selection");
    rectB->setCheckable(true);
    addWidget(rectB);
    _xyproj = new ImageXYProjection(this,_channels,NCHANNELS,*wd.plot());
    connect(rectB, SIGNAL(clicked(bool)), _xyproj, SLOT(setVisible(bool))); }

  { QPushButton* cylB = new QPushButton(QString("%1 / %2 Selection").arg(QChar(0x03c1)).arg(QChar(0x03c6)));
    cylB ->setCheckable(true);
    addWidget(cylB);
    _rfproj = new ImageRPhiProjection(this,_channels,NCHANNELS,*wd.plot());
    connect(cylB, SIGNAL(clicked(bool)), _rfproj, SLOT(setVisible(bool))); }

  { QPushButton* hitB = new QPushButton("Hit Finder");
    hitB ->setCheckable(true);
    addWidget(hitB);
    _hit = new PeakFinder(this,_channels,NCHANNELS,wd);
    connect(hitB, SIGNAL(clicked(bool)), _hit, SLOT(setVisible(bool))); }

  connect(_xyproj, SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_rfproj, SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_hit   , SIGNAL(changed()), this, SLOT(update_configuration()));
}

ImageClient::~ImageClient() {}

void ImageClient::save(char*& p) const
{
  Client::save(p);

  _xyproj->save(p);
  _rfproj->save(p);
  _hit   ->save(p);
}

void ImageClient::load(const char*& p)
{
  Client::load(p);

  _xyproj->load(p);
  _rfproj->load(p);
  _hit   ->load(p);
}

void ImageClient::save_plots(const QString& p) const
{
  _xyproj->save_plots(p+"_xyproj");
  _rfproj->save_plots(p+"_rfproj");
  _hit   ->save_plots(p+"_hits");
}

void ImageClient::_configure(char*& p, 
			     unsigned input, 
			     unsigned& output,
			     ChannelDefinition* ch[], 
			     int* signatures, 
			     unsigned nchannels)
{
   _xyproj->configure(p, input, output, ch, signatures, nchannels);
   _rfproj->configure(p, input, output, ch, signatures, nchannels);
   _hit   ->configure(p, input, output, ch, signatures, nchannels);
}

void ImageClient::_setup_payload(Cds& cds)
{
  _xyproj->setup_payload(cds);
  _rfproj->setup_payload(cds);
  _hit   ->setup_payload(cds);
}

void ImageClient::_update()
{
  _xyproj->update();
  _rfproj->update();
  _hit   ->update();
}
