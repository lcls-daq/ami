#include "ImageXYProjection.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/RectROI.hh"
#include "ami/qt/RectROIDesc.hh"
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
#include "ami/qt/AxisBins.hh"
#include "ami/qt/PostAnalysis.hh"
#include "ami/qt/QtPlotSelector.hh"

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
  _title    (new QLineEdit("Projection"))
{
  _rect = new RectROIDesc(*this,frame,nchannels);

  setWindowTitle("Image Projection");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    _channelBox->addItem(channels[i]->name());
  _channelBox->setCurrentIndex(0);

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
    layout1->addWidget(_channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  layout->addWidget(_rect);
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

  connect(_rect     , SIGNAL(changed()),      this, SIGNAL(changed()));
  connect(_rect     , SIGNAL(rchanged()),     this, SLOT(update_range()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(zoomB     , SIGNAL(clicked()),      this, SLOT(zoom()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(_channelBox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_channel()));
  for(unsigned i=0; i<_nchannels; i++)
    connect(_channels[i], SIGNAL(agg_changed()), this, SLOT(change_channel()));

  _function_plot->plot_desc().post(this, SLOT(add_function_post()));

  ovlyB->setEnabled(false);
  _ovlyB = ovlyB;
  _plotB = plotB;
  connect(_plot_tab,  SIGNAL(currentChanged(int)), this, SLOT(plottab_changed(int)));
}
  
ImageXYProjection::~ImageXYProjection()
{
}

void ImageXYProjection::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert(p, "int", "_channel", QtPersistent::insert(p,_channelBox->currentIndex()) );
  XML_insert(p, "QLineEdit", "_title", QtPersistent::insert(p,_title->text()) );
  XML_insert(p, "QComboBox", "_plot_tab", QtPersistent::insert(p,_plot_tab->currentIndex()) );
  
  XML_insert(p, "XYHistogramPlotDesc", "_histogram_plot", _histogram_plot ->save(p) );
  XML_insert(p, "XYProjectionPlotDesc", "_projection_plot", _projection_plot->save(p) );
  XML_insert(p, "ImageFunctions", "_function_plot", _function_plot  ->save(p) );
  XML_insert(p, "RectROIDesc", "_rect", _rect->save(p) );
}

void ImageXYProjection::load(const char*& p) 
{
  XML_iterate_open(p,tag)
   if (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_channel")
      _channelBox->setCurrentIndex(QtPersistent::extract_i(p));
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
    else if (tag.name == "_rect")
      _rect           ->load(p);
  XML_iterate_close(ImageXYProjection,tag);

  update_range();
}

void ImageXYProjection::save_plots(const QString& p) const
{ _rect->save_plots(p); }

void ImageXYProjection::configure(char*& p, unsigned input, unsigned& output,
				  ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  _rect          ->configure(p,input,output,channels,signatures,nchannels);
  _histogram_plot->configure(p,_channelBox->currentIndex(),output,channels,signatures,nchannels);
}

void ImageXYProjection::setup_payload(Ami::Cds& cds)
{
  _rect          ->setup_payload(cds);
  _histogram_plot->setup_payload(cds);
}

void ImageXYProjection::update()
{
  _rect          ->update();
  _histogram_plot->update();
}

void ImageXYProjection::plot()
{
  RectROI& roi = _rect->roi(_channelBox->currentIndex());
  switch(_plot_tab->currentIndex()) {
  case PlotHistogram:
  case PlotProjection:
    {
      unsigned ppentry = 1;
      AbsOperator* op = _plot_tab->currentIndex()==PlotHistogram ?
        static_cast<AbsOperator*>(_histogram_plot ->desc(qPrintable(_title->text()),ppentry)) :
        static_cast<AbsOperator*>(_projection_plot->desc(qPrintable(_title->text())));
      roi.add_projection(op);
      break;
    }
  case PlotFunction:
    { ScalarPlotDesc* pplot = &_function_plot->plot_desc();
      if (pplot->postAnalysis()) {
        SharedData* post;
        QString qtitle = roi.add_post(pplot->qtitle(),
				      _function_plot->expression(),
				      post);
	DescEntry*  entry  = pplot->desc(qPrintable(qtitle));
	PostAnalysis::instance()->plot(qtitle,entry,post);
      }
      else {
	DescEntry*  desc = pplot->desc(qPrintable(_title->text()));
	roi.add_cursor_plot(new BinMath(*desc,_function_plot->expression()));
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
  RectROI& roi = _rect->roi(_channelBox->currentIndex());
  roi.add_post(_function_plot->plot_desc().qtitle(),
	       _function_plot->expression(),
	       post);
  post->signup();
}

void ImageXYProjection::zoom()
{
  RectROI& roi = _rect->roi(_channelBox->currentIndex());
  roi.add_zoom_plot();  
}

void ImageXYProjection::update_range()
{
  _function_plot->update_range(0,0,INT_MAX,INT_MAX);
}

void ImageXYProjection::plottab_changed(int index)
{
  int ich = _channelBox->currentIndex();
  bool enable = !_channels[ich]->smp_prohibit();
  enable &= index==PlotFunction;
  _ovlyB->setEnabled(enable);
}

void ImageXYProjection::overlay()
{
  switch (_plot_tab->currentIndex()) {
  case PlotFunction:
    { ScalarPlotDesc* pplot = &_function_plot->plot_desc();
      if (pplot->postAnalysis()) {
        SharedData* post;
	RectROI& roi = _rect->roi(_channelBox->currentIndex());
	DescEntry* desc = pplot->desc(qPrintable(roi.add_post(pplot->qtitle(),
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
  RectROI& roi = _rect->roi(_channelBox->currentIndex());
  roi.add_overlay(*plot, new BinMath(*desc,_function_plot->expression()));
  delete desc;
}

void ImageXYProjection::remove_overlay(QtOverlay*) {}

void ImageXYProjection::change_channel()
{
  int ich = _channelBox->currentIndex();
  bool enable = !_channels[ich]->smp_prohibit();
  _plotB->setEnabled(enable);

  plottab_changed(_plot_tab->currentIndex());
}
