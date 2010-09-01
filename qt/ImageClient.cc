#include "ImageClient.hh"

#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageGridScale.hh"
#include "ami/qt/ImageXYProjection.hh"
#include "ami/qt/ImageRPhiProjection.hh"
#include "ami/qt/ImageContourProjection.hh"
#include "ami/qt/PeakFinder.hh"

#include <QtGui/QPushButton>
#include <QtCore/QChar>

using namespace Ami::Qt;

ImageClient::ImageClient(QWidget* parent,const Pds::DetInfo& info, unsigned ch) :
  Client  (parent,info,ch,new ImageDisplay)
{
  ImageDisplay& wd = static_cast<ImageDisplay&>(display());

  { QPushButton* rectB = new QPushButton("X / Y Selection");
    addWidget(rectB);
    _xyproj = new ImageXYProjection(this,_channels,NCHANNELS,*wd.plot());
    connect(rectB, SIGNAL(clicked()), _xyproj, SLOT(show())); }

  { QPushButton* cylB = new QPushButton(QString("%1 / %2 Selection").arg(QChar(0x03c1)).arg(QChar(0x03c6)));
    addWidget(cylB);
    _rfproj = new ImageRPhiProjection(this,_channels,NCHANNELS,*wd.plot());
    connect(cylB, SIGNAL(clicked()), _rfproj, SLOT(show())); }

  { QPushButton* cntB = new QPushButton("Contour Projection");
    addWidget(cntB);
    _cntproj = new ImageContourProjection(this,_channels,NCHANNELS,*wd.plot());
    connect(cntB, SIGNAL(clicked()), _cntproj, SLOT(show())); }

  { QPushButton* hitB = new QPushButton("Hit Finder");
    addWidget(hitB);
    _hit = new PeakFinder(this,_channels,NCHANNELS,wd);
    connect(hitB, SIGNAL(clicked()), _hit, SLOT(show())); }

  connect(_xyproj , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_rfproj , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_cntproj, SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_hit    , SIGNAL(changed()), this, SIGNAL(changed()));
}

ImageClient::~ImageClient() {}

void ImageClient::save(char*& p) const
{
  Client::save(p);

  _xyproj ->save(p);
  _rfproj ->save(p);
  _cntproj->save(p);
  _hit    ->save(p);
}

void ImageClient::load(const char*& p)
{
  Client::load(p);

  _xyproj ->load(p);
  _rfproj ->load(p);
  _cntproj->load(p);
  _hit    ->load(p);

  update_configuration();
}

void ImageClient::save_plots(const QString& p) const
{
  const ImageDisplay& id = static_cast<const ImageDisplay&>(display());
  id.save_plots(p);
  _xyproj ->save_plots(p+"_xyproj");
  _rfproj ->save_plots(p+"_rfproj");
  _cntproj->save_plots(p+"_cntproj");
  _hit    ->save_plots(p+"_hits");
}

void ImageClient::_configure(char*& p, 
			     unsigned input, 
			     unsigned& output,
			     ChannelDefinition* ch[], 
			     int* signatures, 
			     unsigned nchannels)
{
   _xyproj ->configure(p, input, output, ch, signatures, nchannels);
   _rfproj ->configure(p, input, output, ch, signatures, nchannels);
   _cntproj->configure(p, input, output, ch, signatures, nchannels);
   _hit    ->configure(p, input, output, ch, signatures, nchannels);
}

void ImageClient::_setup_payload(Cds& cds)
{
  _xyproj ->setup_payload(cds);
  _rfproj ->setup_payload(cds);
  _cntproj->setup_payload(cds);
  _hit    ->setup_payload(cds);
  static_cast<ImageDisplay&>(display()).grid_scale().setup_payload(cds);
}

void ImageClient::_update()
{
  _xyproj ->update();
  _rfproj ->update();
  _cntproj->update();
  _hit    ->update();
}
