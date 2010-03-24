#include "ImageXYProjection.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/RectangleCursors.hh"
#include "ami/qt/ProjectionPlot.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/ZoomPlot.hh"
#include "ami/qt/XYProjectionPlotDesc.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/qt/Display.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/AxisBins.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/Entry.hh"
#include "ami/data/BinMath.hh"
#include "ami/data/XYProjection.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>
#include <QtGui/QTabWidget>

#include <sys/socket.h>

using namespace Ami::Qt;

enum { PlotProjection, PlotIntegral };

ImageXYProjection::ImageXYProjection(QWidget*           parent,
				     ChannelDefinition* channels[],
				     unsigned           nchannels, 
				     ImageFrame&        frame) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _title    (new QLineEdit("Projection"))
{
  _rectangle = new RectangleCursors(_frame);

  setWindowTitle("Image Projection");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  QPushButton* zoomB  = new QPushButton("Zoom");
  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");

  _plot_tab        = new QTabWidget(0);
  _projection_plot = new XYProjectionPlotDesc(0, *_rectangle);
  _integral_plot   = new ScalarPlotDesc(0);
  _plot_tab->insertTab(PlotProjection,_projection_plot,"Projection");
  _plot_tab->insertTab(PlotIntegral  ,_integral_plot  ,"Integral"); 

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* locations_box = new QGroupBox("Define Boundaries");
    locations_box->setToolTip("Define projection boundaries.");
    QVBoxLayout* layout2 = new QVBoxLayout;
    layout2->addWidget(_rectangle);
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QGroupBox* plot_box = new QGroupBox("Plot");
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Title"));
      layout2->addWidget(_title);
      layout1->addLayout(layout2); }
    layout1->addWidget(_plot_tab);
    plot_box->setLayout(layout1);
    layout->addWidget(plot_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(zoomB);
    layout1->addWidget(plotB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }

  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(zoomB     , SIGNAL(clicked()),      this, SLOT(zoom()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
}
  
ImageXYProjection::~ImageXYProjection()
{
}

void ImageXYProjection::save(char*& p) const
{
  QtPWidget::save(p);

  QtPersistent::insert(p,_channel);
  QtPersistent::insert(p,_title->text());
  QtPersistent::insert(p,_plot_tab->currentIndex());
  
  _projection_plot->save(p);
  _integral_plot  ->save(p);

  _rectangle->save(p);

  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++) {
    QtPersistent::insert(p,QString("ProjectionPlot"));
    (*it)->save(p);
  }

  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++) {
    QtPersistent::insert(p,QString("CursorPlot"));
    (*it)->save(p);
  }

  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++) {
    QtPersistent::insert(p,QString("ZoomPlot"));
    (*it)->save(p);
  }

  QtPersistent::insert(p,QString("EndImageXYProjection"));
}

void ImageXYProjection::load(const char*& p) 
{
  QtPWidget::load(p);
  
  _channel = QtPersistent::extract_i(p);
  _title->setText(QtPersistent::extract_s(p));
  _plot_tab->setCurrentIndex(QtPersistent::extract_i(p));

  _projection_plot->load(p);
  _integral_plot  ->load(p);

  _rectangle->load(p);

  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _pplots.clear();

  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _cplots.clear();

  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _zplots.clear();

  QString name = QtPersistent::extract_s(p);
  while(name == QString("ProjectionPlot")) {
    ProjectionPlot* plot = new ProjectionPlot(this, p);
    _pplots.push_back(plot);
    connect(plot, SIGNAL(description_changed()), this, SLOT(configure_plot()));
    connect(plot, SIGNAL(destroyed(QObject*))  , this, SLOT(remove_plot(QObject*)));

    name = QtPersistent::extract_s(p);
  }
  while(name == QString("CursorPlot")) {
    CursorPlot* plot = new CursorPlot(this, p);
    _cplots.push_back(plot);
    connect(plot, SIGNAL(destroyed(QObject*))  , this, SLOT(remove_plot(QObject*)));

    name = QtPersistent::extract_s(p);
  }
  while(name == QString("ZoomPlot")) {
    ZoomPlot* plot = new ZoomPlot(this, p);
    _zplots.push_back(plot);
    connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

    name = QtPersistent::extract_s(p);
  }
}

void ImageXYProjection::save_plots(const QString& p) const
{
}

void ImageXYProjection::setVisible(bool v)
{
  if (v)    _frame.add_marker(*_rectangle);
  else      _frame.remove_marker(*_rectangle);
  QWidget::setVisible(v);
}

void ImageXYProjection::configure(char*& p, unsigned input, unsigned& output,
				  ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  const unsigned maxpixels=1024;
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels);
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,
		     AxisBins(0,maxpixels,maxpixels),Ami::ConfigureRequest::Analysis);
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels);
}

void ImageXYProjection::setup_payload(Cds& cds)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
   (*it)->setup_payload(cds);
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
   (*it)->setup_payload(cds);
}

void ImageXYProjection::update()
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->update();
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->update();
}

void ImageXYProjection::set_channel(int c) 
{ 
  _channel=c; 
}

void ImageXYProjection::plot()
{
  switch(_plot_tab->currentIndex()) {
  case PlotProjection:
    { ProjectionPlot* plot = 
	new ProjectionPlot(this,_title->text(), _channel, 
			   _projection_plot->desc(qPrintable(_title->text())));
      
      _pplots.push_back(plot);

      connect(plot, SIGNAL(description_changed()), this, SLOT(configure_plot()));
      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
      emit changed();

      break;
    }
  case PlotIntegral:
    { QString expr = QString("[%1]%2[%3][%4]%5[%6]").
	arg(unsigned(_rectangle->xlo())).
	arg(BinMath::integrate()).
	arg(unsigned(_rectangle->xhi())).
	arg(unsigned(_rectangle->ylo())).
	arg(BinMath::integrate()).
	arg(unsigned(_rectangle->yhi()));
      
      DescEntry*  desc = _integral_plot->desc(qPrintable(_title->text()));
      CursorPlot* plot = 
 	new CursorPlot(this, _title->text(), _channel, new BinMath(*desc,qPrintable(expr)));
      
      _cplots.push_back(plot);

      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
      emit changed();

      break;
    }
  default:
    return;
  }

}

void ImageXYProjection::zoom()
{
  ZoomPlot* plot = new ZoomPlot(this,
				_channels[_channel]->name(),
				_channel,
				unsigned(_rectangle->xlo()),
				unsigned(_rectangle->ylo()),
				unsigned(_rectangle->xhi()),
				unsigned(_rectangle->yhi()));

  _zplots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void ImageXYProjection::remove_plot(QObject* obj)
{
  { ProjectionPlot* plot = static_cast<ProjectionPlot*>(obj);
    _pplots.remove(plot); }

  { CursorPlot* plot = static_cast<CursorPlot*>(obj);
    _cplots.remove(plot); }

  { ZoomPlot* plot = static_cast<ZoomPlot*>(obj);
    _zplots.remove(plot); }

  disconnect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

void ImageXYProjection::configure_plot()
{
  emit changed();
}
