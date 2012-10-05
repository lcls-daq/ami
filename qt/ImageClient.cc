#include "ami/qt/Client.hh"
#include "ami/qt/ImageClient.hh"
#include "ami/qt/Control.hh"

#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageGridScale.hh"
#include "ami/qt/ImageXYProjection.hh"
#include "ami/qt/ImageRPhiProjection.hh"
#include "ami/qt/ImageContourProjection.hh"
#include "ami/qt/PeakFinder.hh"
#include "ami/qt/BlobFinder.hh"

#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtCore/QChar>

using namespace Ami::Qt;
class ImageDisplay;
class Control;

Ami::Qt::ImageClient::ImageClient(QWidget* parent,const Pds::DetInfo& info, unsigned ch) :
  Client  (parent,info,ch,new ImageDisplay,1.)
{
  ImageDisplay& wd = (ImageDisplay&)(display());
  connect(&wd, SIGNAL(set_chrome_visible(bool)), this, SLOT(set_chrome_visible(bool)));

  { QPushButton* rectB = new QPushButton("X / Y Selection");
    addWidget(rectB);
    _xyproj = new ImageXYProjection(this,_channels,NCHANNELS,*wd.plot());
    connect(rectB, SIGNAL(clicked()), _xyproj, SLOT(front())); }

  { QPushButton* cylB = new QPushButton(QString("%1 / %2 Selection").arg(QChar(0x03c1)).arg(QChar(0x03c6)));
    addWidget(cylB);
    _rfproj = new ImageRPhiProjection(this,_channels,NCHANNELS,*wd.plot());
    connect(cylB, SIGNAL(clicked()), _rfproj, SLOT(front())); }

  { QPushButton* cntB = new QPushButton("Contour Projection");
    addWidget(cntB);
    _cntproj = new ImageContourProjection(this,_channels,NCHANNELS,*wd.plot());
    connect(cntB, SIGNAL(clicked()), _cntproj, SLOT(front())); }

  { QPushButton* hitB = new QPushButton("Hit Finder");
    addWidget(hitB);
    _hit = new PeakFinder(this,_channels,NCHANNELS);
    connect(hitB, SIGNAL(clicked()), _hit, SLOT(front()));}

  { QPushButton* blobB = new QPushButton("Blob Finder");
    addWidget(blobB);
    _blob = new BlobFinder(this,_channels,NCHANNELS,*wd.plot());
    connect(blobB, SIGNAL(clicked()), _blob, SLOT(front()));}

  connect(_xyproj , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_rfproj , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_cntproj, SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_hit    , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_blob   , SIGNAL(changed()), this, SIGNAL(changed()));
}

Ami::Qt::ImageClient::~ImageClient() {}

void Ami::Qt::ImageClient::save(char*& p) const
{
  XML_insert(p, "Client", "self", Client::save(p) );

  XML_insert(p, "ImageXYProjection", "_xyproj", _xyproj ->save(p) );
  XML_insert(p, "ImageRPhiProjection", "_rfproj", _rfproj ->save(p) );
  XML_insert(p, "ImageContourProjection", "_cntproj", _cntproj->save(p) );
  XML_insert(p, "PeakFinder", "_hit", _hit    ->save(p) );
  XML_insert(p, "BlobFinder", "_blob", _blob  ->save(p) );
}

void Ami::Qt::ImageClient::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.element == "Client")
      Client::load(p);
    else if (tag.name == "_xyproj")
      _xyproj ->load(p);
    else if (tag.name == "_rfproj")
      _rfproj ->load(p);
    else if (tag.name == "_cntproj")
      _cntproj->load(p);
    else if (tag.name == "_hit")
      _hit    ->load(p);
    else if (tag.name == "_blob")
      _blob   ->load(p);
  XML_iterate_close(ImageClient,tag);

  update_configuration();
}

void Ami::Qt::ImageClient::save_plots(const QString& p) const
{
  const ImageDisplay& id = static_cast<const ImageDisplay&>(display());
  id.save_plots(p);
  _xyproj ->save_plots(p+"_xyproj");
  _rfproj ->save_plots(p+"_rfproj");
  _cntproj->save_plots(p+"_cntproj");
  _hit    ->save_plots(p+"_hits");
  _blob   ->save_plots(p+"_blob");
}

void Ami::Qt::ImageClient::_configure(char*& p,
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
   _blob   ->configure(p, input, output, ch, signatures, nchannels);
}

void Ami::Qt::ImageClient::_setup_payload(Cds& cds)
{
  _xyproj ->setup_payload(cds);
  _rfproj ->setup_payload(cds);
  _cntproj->setup_payload(cds);
  _hit    ->setup_payload(cds);
  _blob   ->setup_payload(cds);
  static_cast<ImageDisplay&>(display()).grid_scale().setup_payload(cds);
}

void Ami::Qt::ImageClient::_update()
{
  _xyproj ->update();
  _rfproj ->update();
  _cntproj->update();
  _hit    ->update();
  _blob   ->update();
}

void Ami::Qt::ImageClient::_prototype(const DescEntry& e)
{
  _hit    ->prototype(e);
}
