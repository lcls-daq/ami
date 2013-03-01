#include "ImageXYProjection.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/RectangleCursors.hh"
#include "ami/qt/ProjectionPlot.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/CursorPost.hh"
#include "ami/qt/CursorOverlay.hh"
#include "ami/qt/ZoomPlot.hh"
#include "ami/qt/XYHistogramPlotDesc.hh"
#include "ami/qt/XYProjectionPlotDesc.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/qt/ImageFunctions.hh"
#include "ami/qt/Display.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/PostAnalysis.hh"
#include "ami/qt/QtPlotSelector.hh"
#include "ami/qt/RectROI.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/Entry.hh"
#include "ami/data/BinMath.hh"
#include "ami/data/XYHistogram.hh"
#include "ami/data/XYProjection.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>
#include <QtGui/QTabWidget>

#include <sys/socket.h>
#include <limits.h>

using namespace Ami::Qt;

enum { PlotHistogram, PlotProjection, PlotFunction };


ImageXYProjection::ImageXYProjection(QtPWidget*         parent,
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
  _rectangle = new RectangleCursors(_frame, parent);

  setWindowTitle("Image Projection");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _roiButton = new QPushButton("New");
  _roiBox = new QComboBox;
  new_roi();

  QPushButton* zoomB  = new QPushButton("Zoom");
  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* ovlyB  = new QPushButton("Overlay");
  QPushButton* closeB = new QPushButton("Close");

  _plot_tab        = new QTabWidget(0);
  _histogram_plot  = new XYHistogramPlotDesc (0);
  _projection_plot = new XYProjectionPlotDesc(0);
  _function_plot   = new ImageFunctions(0);
  _plot_tab->insertTab(PlotHistogram ,_histogram_plot ,"Histogram");
  _plot_tab->insertTab(PlotProjection,_projection_plot,"Projection");
  _plot_tab->insertTab(PlotFunction  ,_function_plot  ,"Function"); 

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox;
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Source Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* locations_box = new QGroupBox("Region of Interest");
    locations_box->setToolTip("Define projection boundaries.");
    QVBoxLayout* layout2 = new QVBoxLayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addStretch();
      layout1->addWidget(new QLabel("Select"));
      layout1->addWidget(_roiBox);
      layout1->addWidget(_roiButton);
      layout1->addStretch();
      layout2->addLayout(layout1); }
    layout2->addWidget(_rectangle);
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QGroupBox* plot_box = new QGroupBox;
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Plot Title"));
      layout2->addWidget(_title);
      layout1->addLayout(layout2); }
    layout1->addWidget(_plot_tab);
    plot_box->setLayout(layout1);
    layout->addWidget(plot_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(zoomB);
    layout1->addWidget(plotB);
    layout1->addWidget(ovlyB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }

  setLayout(layout);

  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(_rectangle, SIGNAL(changed()),      this, SLOT(update_range()));
  connect(_rectangle, SIGNAL(edited()),       this, SIGNAL(changed()));
  connect(_rectangle, SIGNAL(done()),         this, SLOT(front()));
  connect(_rectangle, SIGNAL(done()),         this, SIGNAL(changed()));
  connect(_roiButton, SIGNAL(clicked()),      this, SLOT(new_roi()));
  connect(_roiBox   , SIGNAL(currentIndexChanged(int)), this, SLOT(select_roi(int)));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(zoomB     , SIGNAL(clicked()),      this, SLOT(zoom()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));

  _function_plot->plot_desc().post(this, SLOT(add_function_post()));

  ovlyB->setEnabled(false);
  _ovlyB = ovlyB;
  connect(_plot_tab,  SIGNAL(currentChanged(int)), this, SLOT(plottab_changed(int)));
}
  
ImageXYProjection::~ImageXYProjection()
{
  for(unsigned i=0; i<_rect.size(); i++)
    delete _rect[i];
  _rect.clear();

  for(unsigned i=0; i<_rois.size(); i++)
    delete _rois[i];
  _rois.clear();
}

void ImageXYProjection::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert(p, "int", "_channel", QtPersistent::insert(p,_channel) );
  XML_insert(p, "QLineEdit", "_title", QtPersistent::insert(p,_title->text()) );
  XML_insert(p, "QComboBox", "_plot_tab", QtPersistent::insert(p,_plot_tab->currentIndex()) );
  
  XML_insert(p, "XYHistogramPlotDesc", "_histogram_plot", _histogram_plot ->save(p) );
  XML_insert(p, "XYProjectionPlotDesc", "_projection_plot", _projection_plot->save(p) );
  XML_insert(p, "ImageFunctions", "_function_plot", _function_plot  ->save(p) );

  XML_insert(p, "RectangleCursors", "_rectangle", _rectangle->save(p) );

  for(unsigned i=0; i<_rect.size(); i++) {
    XML_insert(p, "Rect", "_rect", _rect[i]->save(p) );
  }
  for(unsigned i=0; i<_rois.size(); i++) {
    XML_insert(p, "RectROI", "_rois", _rois[i]->save(p) );
  }
}

void ImageXYProjection::load(const char*& p) 
{
  for(unsigned i=0; i<_rect.size(); i++)
    delete _rect[i];
  _rect.clear();

  for(unsigned i=0; i<_rois.size(); i++)
    delete _rois[i];
  _rois.clear();

  _roiBox->clear();

  XML_iterate_open(p,tag)
   if (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_title")
      _title->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_plot_tab")
      _plot_tab->setCurrentIndex(QtPersistent::extract_i(p));
    else if (tag.name == "_histogram_plot")
      _histogram_plot ->load(p);
    else if (tag.name == "_projection_plot")
      _projection_plot->load(p);
    else if (tag.name == "_function_plot")
      _function_plot  ->load(p);
    else if (tag.name == "_rectangle")
      _rectangle->load(p);
    else if (tag.name == "_rect") {
      Rect* r = new Rect;
      r->load(p); 
      _rect.push_back(r);
    }
    else if (tag.name == "_rois") {
      int rect = _rois.size()/_nchannels;
      QString name = QString("ROI%1").arg(rect);
      unsigned channel = _rois.size()%_nchannels;
      if (channel==0)
        _roiBox->addItem(name);
      RectROI* roi = new RectROI(this, name, channel, *_rect[rect]);
      roi->load(p);
      _rois.push_back(roi);
      connect(roi, SIGNAL(changed()), this, SIGNAL(changed()));
    }
  XML_iterate_close(ImageXYProjection,tag);

  update_range();
}

void ImageXYProjection::save_plots(const QString& p) const
{
  for(unsigned i=0; i<_rois.size(); i++) {
    QString q = QString("%1_ROI%2_%3").arg(p).arg(i/4).arg(i%4);
    _rois[i]->save_plots(q);
  }
}

void ImageXYProjection::configure(char*& p, unsigned input, unsigned& output,
				  ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  for(std::vector<RectROI*>::iterator it=_rois.begin(); it!=_rois.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels);
    
  _histogram_plot->configure(p,_channel,output,channels,signatures,nchannels);
}

void ImageXYProjection::setup_payload(Ami::Cds& cds)
{
  for(unsigned i=0; i<_rois.size(); i++) 
    _rois[i]->setup_payload(cds);

  _histogram_plot->setup_payload(cds);
}

void ImageXYProjection::update()
{
  for(unsigned i=0; i<_rois.size(); i++)
    _rois[i]->update();

  _histogram_plot->update();
}

void ImageXYProjection::set_channel(int c) 
{ 
  _channel=c; 
}

void ImageXYProjection::plot()
{
  switch(_plot_tab->currentIndex()) {
  case PlotHistogram:
  case PlotProjection:
    { AbsOperator* op = _plot_tab->currentIndex()==PlotHistogram ?
        static_cast<AbsOperator*>(_histogram_plot ->desc(qPrintable(_title->text()))) :
        static_cast<AbsOperator*>(_projection_plot->desc(qPrintable(_title->text())));
      _roi().add_projection(op);
      break;
    }
  case PlotFunction:
    { ScalarPlotDesc* pplot = &_function_plot->plot_desc();
      if (pplot->postAnalysis()) {
        SharedData* post;
        QString qtitle = _roi().add_post(pplot->qtitle(),
                                         _function_plot->expression(),
                                         post);
	DescEntry*  entry  = pplot->desc(qPrintable(qtitle));
	PostAnalysis::instance()->plot(qtitle,entry,post);
      }
      else {
	DescEntry*  desc = pplot->desc(qPrintable(_title->text()));
        _roi().add_cursor_plot(new BinMath(*desc,_function_plot->expression()));
	delete desc;
      }
    } 
    break;
  default:
    return;
  }
}

void ImageXYProjection::add_function_post() 
{
  SharedData* post;
  _roi().add_post(_function_plot->plot_desc().qtitle(),
                  _function_plot->expression(),
                  post);
  post->signup();
}

void ImageXYProjection::zoom()
{
  _roi().add_zoom_plot();  
}

void ImageXYProjection::update_range()
{
  _function_plot->update_range(0,0,INT_MAX,INT_MAX);

  _frame.replot();

  if (_roiBox->currentIndex()>=0)
    _rectangle->save(*_rect[_roiBox->currentIndex()]);
}

void ImageXYProjection::plottab_changed(int index)
{
  _ovlyB->setEnabled(index==PlotFunction);
}

void ImageXYProjection::overlay()
{
  switch (_plot_tab->currentIndex()) {
  case PlotFunction:
    { ScalarPlotDesc* pplot = &_function_plot->plot_desc();
      if (pplot->postAnalysis()) {
        SharedData* post;
	DescEntry* desc = pplot->desc(qPrintable(_roi().add_post(pplot->qtitle(),
                                                                 _function_plot->expression(),
                                                                 post)));
	new QtPlotSelector(*this, *PostAnalysis::instance(), desc, post);
      }
      else {
	DescEntry* desc = pplot->desc(qPrintable(_title->text()));
	new QtPlotSelector(*this, *this, desc);
      }
    }
    break;
  default:
    break;
  }
}

void ImageXYProjection::add_overlay(DescEntry* desc, QtPlot* plot, SharedData*)
{
  _roi().add_overlay(*plot, new BinMath(*desc,_function_plot->expression()));
  delete desc;
}

void ImageXYProjection::remove_overlay(QtOverlay*) {}

void ImageXYProjection::showEvent(QShowEvent* ev)
{
  QtPWidget::showEvent(ev);
  _frame.add_marker(*_rectangle);
  update_range();
}

void ImageXYProjection::hideEvent(QHideEvent* ev)
{
  QtPWidget::hideEvent(ev);
  _frame.remove_marker(*_rectangle);
  update_range();
}

void ImageXYProjection::new_roi()
{
  int i=_rois.size()/_nchannels;
  _rect.push_back(new Rect());
  QString name = QString("ROI%1").arg(i);
  _roiBox->addItem(name);
  for(unsigned j=0; j<_nchannels; j++) {
    RectROI* roi = new RectROI(this, name,j,*_rect[i]);
    _rois.push_back(roi);
    connect(roi, SIGNAL(changed()), this, SIGNAL(changed()));
  }
  _roiBox->setCurrentIndex(i);
}

void ImageXYProjection::select_roi(int i)
{
  if (i>=0)
    _rectangle->load(*_rect[i]);
}

RectROI& ImageXYProjection::_roi() { return *_rois[_roiBox->currentIndex()*_nchannels + _channel]; }
