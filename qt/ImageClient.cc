#include "ami/qt/Client.hh"
#include "ami/qt/ImageClient.hh"
#include "ami/qt/Control.hh"

#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageGridScale.hh"
#include "ami/qt/ImageXYProjection.hh"
#include "ami/qt/ImageRPhiProjection.hh"
#include "ami/qt/ImageContourProjection.hh"
#include "ami/qt/PeakFinder.hh"

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
  wd.container(this);
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
    connect(hitB, SIGNAL(clicked()), _hit, SLOT(show()));}

  connect(_xyproj , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_rfproj , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_cntproj, SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_hit    , SIGNAL(changed()), this, SIGNAL(changed()));
}

Ami::Qt::ImageClient::~ImageClient() {}

void Ami::Qt::ImageClient::save(char*& p) const
{
  XML_insert(p, "Client", "self", Client::save(p) );

  XML_insert(p, "ImageXYProjection", "_xyproj", _xyproj ->save(p) );
  XML_insert(p, "ImageRPhiProjection", "_rfproj", _rfproj ->save(p) );
  XML_insert(p, "ImageContourProjection", "_cntproj", _cntproj->save(p) );
  XML_insert(p, "PeakFinder", "_hit", _hit    ->save(p) );
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
}

void Ami::Qt::ImageClient::hideWidgets()
{
  unsigned i = 0;
  QWidget* wid = 0;
  if (_layout->parentWidget() != 0) {
    wid = _layout->parentWidget()->window();
    printf("ImageClient::_layout has a parent widget window %d,%d before hide\n",
        wid->minimumWidth(), wid->minimumHeight());
  }
  QLayoutItem* item;
  while ((item = _layout3->itemAt(i++))) {
    if (item->widget()) {
      item->widget()->hide();
    }
  }
  i = 0;
  while ((item = _layout->itemAt(i++))) {
    if (item->widget()) {
      item->widget()->hide();
    }
  }
  if (wid) {
    wid->updateGeometry();
    printf("ImageClient::_layout parent window widget %d,%d after hide\n", wid->minimumWidth(), wid->minimumHeight());
    wid->resize(wid->minimumWidth(), wid->minimumHeight());
  }
}

void Ami::Qt::ImageClient::showWidgets()
{
  unsigned i = 0;
  QLayoutItem* item;
  while ((item = _layout3->itemAt(i++))) {
    if (item->widget()) {
      item->widget()->show();
    }
  }
  i = 0;
  while ((item = _layout->itemAt(i++))) {
    if (item->widget()) {
      item->widget()->show();
    }
  }
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
}

void Ami::Qt::ImageClient::_setup_payload(Cds& cds)
{
  _xyproj ->setup_payload(cds);
  _rfproj ->setup_payload(cds);
  _cntproj->setup_payload(cds);
  _hit    ->setup_payload(cds);
  static_cast<ImageDisplay&>(display()).grid_scale().setup_payload(cds);
}

void Ami::Qt::ImageClient::_update()
{
  _xyproj ->update();
  _rfproj ->update();
  _cntproj->update();
  _hit    ->update();
}

void Ami::Qt::ImageClient::_prototype(const DescEntry& e)
{
  _hit    ->prototype(e);
}
