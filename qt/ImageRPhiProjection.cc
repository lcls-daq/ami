#include "ImageRPhiProjection.hh"

#include "ami/qt/AnnulusCursors.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/ProjectionPlot.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/CursorPost.hh"
#include "ami/qt/CursorOverlay.hh"
#include "ami/qt/RPhiProjectionPlotDesc.hh"
#include "ami/qt/ImageIntegral.hh"
#include "ami/qt/ImageContrast.hh"
#include "ami/qt/Display.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/PostAnalysis.hh"
#include "ami/qt/QtPlotSelector.hh"

#include "ami/data/BinMath.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/Entry.hh"
#include "ami/data/RPhiProjection.hh"
#include "ami/data/FFT.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QMessageBox>
#include <QtGui/QTabWidget>

#include <sys/socket.h>

using namespace Ami::Qt;

enum { PlotProjection, PlotIntegral, PlotContrast };

ImageRPhiProjection::ImageRPhiProjection(QtPWidget*         parent,
					 ChannelDefinition* channels[],
					 unsigned           nchannels, 
					 ImageFrame&        frame) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _annulus  (new AnnulusCursors(frame, parent)),
  _title    (new QLineEdit("Projection"))
{
  setWindowTitle("Image Projection");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* ovlyB  = new QPushButton("Overlay");
  QPushButton* closeB = new QPushButton("Close");

  _plot_tab        = new QTabWidget(0);
  _projection_plot = new RPhiProjectionPlotDesc(0, *_annulus);
  _integral_plot   = new ImageIntegral(0);
  _contrast_plot   = new ImageContrast(0);
  _plot_tab->insertTab(PlotProjection,_projection_plot,"Projection");
  _plot_tab->insertTab(PlotIntegral  ,_integral_plot  ,"Integral"); 
  _plot_tab->insertTab(PlotContrast  ,_contrast_plot  ,"Contrast"); 

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox;
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Source Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* boundary_box = new QGroupBox("Region of Interest");
    QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(_annulus);
    boundary_box->setLayout(layout1);
    layout->addWidget(boundary_box); }
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
    layout1->addWidget(plotB);
    layout1->addWidget(ovlyB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(_annulus  , SIGNAL(changed()),      this, SLOT(update_range()));
  connect(_annulus  , SIGNAL(done()),         this, SLOT(front()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));

  _integral_plot->post(this, SLOT(add_integral_post()));
  _contrast_plot->post(this, SLOT(add_contrast_post()));

  ovlyB->setEnabled(false);
  _ovlyB = ovlyB;
  connect(_plot_tab,  SIGNAL(currentChanged(int)), this, SLOT(plottab_changed(int)));
}
  
ImageRPhiProjection::~ImageRPhiProjection()
{
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
}

void ImageRPhiProjection::save(char*& p) const
{
  XML_insert( p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert( p, "unsigned", "_channel", QtPersistent::insert(p,_channel) );
  XML_insert( p, "QLineEdit", "_title" , QtPersistent::insert(p,_title->text()) );
  XML_insert( p, "QTabWidget", "_plot_tab", QtPersistent::insert(p,_plot_tab->currentIndex()) );

  XML_insert( p, "RPhiProjectionPlotDesc", "_projection_plot", _projection_plot->save(p) );
  XML_insert( p, "ImageIntegral", "_integral_plot", _integral_plot->save(p) );
  XML_insert( p, "ImageContrast", "_contrast_plot", _contrast_plot->save(p) );

  XML_insert( p, "AnnulusCursors", "_annulus", _annulus->save(p) );

  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++) {
    XML_insert( p, "ProjectionPlot", "_pplots", (*it)->save(p) );
  }

  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++) {
    XML_insert( p, "CursorPlot", "_cplots", (*it)->save(p) );
  }
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    XML_insert(p, "CursorPost", "_posts", (*it)->save(p) );
  }
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++) {
    XML_insert( p, "CursorOverlay", "_ovls",
                (*it)->save(p) );
  }
}

void ImageRPhiProjection::load(const char*& p)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _pplots.clear();

  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _cplots.clear();
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  _ovls .clear();

  XML_iterate_open(p,tag)
    if (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_title")
      _title->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_plot_tab")
      _plot_tab->setCurrentIndex(QtPersistent::extract_i(p));
    else if (tag.name == "_projection_plot")
      _projection_plot->load(p);
    else if (tag.name == "_integral_plot")
      _integral_plot  ->load(p);
    else if (tag.name == "_contrast_plot")
      _contrast_plot  ->load(p);
    else if (tag.name == "_annulus")
      _annulus->load(p);
    else if (tag.name == "_pplots") {
      ProjectionPlot* plot = new ProjectionPlot(this, p);
      _pplots.push_back(plot);
      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_cplots") {
      CursorPlot* plot = new CursorPlot(this, p);
      _cplots.push_back(plot);
      connect(plot, SIGNAL(destroyed(QObject*))  , this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_posts") {
      CursorPost* post = new CursorPost(p);
      _posts.push_back(post);
    }
    else if (tag.name == "_ovls") {
      CursorOverlay* ovl = new CursorOverlay(*this, p);
      _ovls.push_back(ovl);
    }
  XML_iterate_close(ImageRPhiProjection,tag);
}

void ImageRPhiProjection::save_plots(const QString& p) const
{
  int i=1;
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->save_plots(QString("%1_%2").arg(p).arg(i++));
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg(i++);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
}

void ImageRPhiProjection::setVisible(bool v)
{
  if (v)  _frame.add_marker(*_annulus);
  else    _frame.remove_marker(*_annulus);
  QWidget::setVisible(v);
  update_range();
}

void ImageRPhiProjection::configure(char*& p, unsigned input, unsigned& output,
				ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  const unsigned maxint=0x40000000;
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels);
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,
		     AxisBins(0,maxint,maxint),Ami::ConfigureRequest::Analysis);
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,
		     AxisBins(0,maxint,maxint),Ami::ConfigureRequest::Analysis);
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,
		     AxisBins(0,maxint,maxint),Ami::ConfigureRequest::Analysis);
}

void ImageRPhiProjection::setup_payload(Cds& cds)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
   (*it)->setup_payload(cds);
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
   (*it)->setup_payload(cds);
}

void ImageRPhiProjection::update()
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->update();
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
}

void ImageRPhiProjection::set_channel(int c) 
{ 
  _channel=c; 
}

void ImageRPhiProjection::plot()
{
#define CASE_PLOT(pplot) {                                              \
    if (pplot->postAnalysis()) {                                        \
      QString     qtitle = _add_post(pplot->qtitle(),pplot->expression()); \
      DescEntry*  entry  = pplot->desc(qPrintable(qtitle));             \
      PostAnalysis::instance()->plot(qtitle,entry,_posts.back());       \
    }                                                                   \
    else {                                                              \
      DescEntry*  desc = pplot->desc(qPrintable(_title->text()));       \
      CursorPlot* plot =                                                \
        new CursorPlot(this, _title->text(), _channel,                  \
                       new BinMath(*desc,pplot->expression()));         \
      delete desc;                                                      \
      _cplots.push_back(plot);                                          \
      connect(plot, SIGNAL(changed()), this, SIGNAL(changed()));        \
      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*))); \
    }                                                                   \
    emit changed();                                                     \
  }

  double f0 = _annulus->phi0();
  double f1 = _annulus->phi1();
  if (!(f0 < f1)) f1 += 2*M_PI;

  switch(_plot_tab->currentIndex()) {
  case PlotProjection:
    { ProjectionPlot* plot = 
	new ProjectionPlot(this,_title->text(), _channel, 
			   _projection_plot->desc(qPrintable(_title->text())));

      _pplots.push_back(plot);

      connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
      emit changed();

      break;
    }
  case PlotIntegral:
    CASE_PLOT(_integral_plot);
    break;
  case PlotContrast:
    CASE_PLOT(_contrast_plot);
    break;
  default:
    break;
  }

#undef CASE_PLOT
}

void ImageRPhiProjection::add_integral_post() 
{
  _add_post(_integral_plot->qtitle(),_integral_plot->expression()); 
  _posts.back()->signup();
}

void ImageRPhiProjection::add_contrast_post() 
{
  _add_post(_contrast_plot->qtitle(),_contrast_plot->expression()); 
  _posts.back()->signup();
}

QString ImageRPhiProjection::_add_post(const QString& title, const char* expr)
{
  QString qtitle = FeatureRegistry::instance(Ami::PostAnalysis).validate_name(title);

  Ami::DescCache*  desc = new Ami::DescCache(qPrintable(qtitle),
                                             qPrintable(qtitle),
                                             Ami::PostAnalysis);
  CursorPost* post = new CursorPost(_channel,
				    new BinMath(*desc,expr),
                                    this);
  _posts.push_back(post);

  delete desc;

  emit changed();

  return qtitle;
}

void ImageRPhiProjection::remove_plot(QObject* obj)
{
  { ProjectionPlot* plot = static_cast<ProjectionPlot*>(obj);
    _pplots.remove(plot); }

  { CursorPlot* plot = static_cast<CursorPlot*>(obj);
    _cplots.remove(plot); }

  disconnect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

void ImageRPhiProjection::update_range()
{
  _integral_plot->update_range(_annulus->xcenter(),
                               _annulus->ycenter(),
                               _annulus->r_inner(),
                               _annulus->r_outer(),
                               _annulus->phi0(),
                               _annulus->phi1());
  _contrast_plot->update_range(_annulus->xcenter(),
                               _annulus->ycenter(),
                               _annulus->r_inner(),
                               _annulus->r_outer(),
                               _annulus->phi0(),
                               _annulus->phi1());

  _frame.replot();
}

void ImageRPhiProjection::remove_cursor_post(CursorPost* post)
{
  _posts.remove(post);
  emit changed();
}

void ImageRPhiProjection::plottab_changed(int index)
{
  _ovlyB->setEnabled(index==PlotIntegral ||
                     index==PlotContrast);
}

void ImageRPhiProjection::overlay()
{
#define CASE_PLOT(pplot) {                                              \
    if (pplot->postAnalysis()) {                                        \
      DescEntry* desc = pplot->desc(qPrintable(_add_post(pplot->qtitle(),pplot->expression()))); \
      new QtPlotSelector(*this, *PostAnalysis::instance(), desc, _posts.back()); \
    }                                                                   \
    else {                                                              \
      DescEntry* desc = pplot->desc(qPrintable(_title->text()));        \
      new QtPlotSelector(*this, *this, desc);                           \
    }                                                                   \
  }

  switch(_plot_tab->currentIndex()) {
  case PlotIntegral:
    CASE_PLOT(_integral_plot);
    break;
  case PlotContrast:
    CASE_PLOT(_contrast_plot);
    break;
  default:
    break;
  }

#undef CASE_PLOT
}

void ImageRPhiProjection::add_overlay(DescEntry* desc,
                                      QtPlot*    plot,
                                      SharedData*)
{
  CursorOverlay* ovl = new CursorOverlay(*this, 
                                         *plot,
                                         _channel, 
                                         new BinMath(*desc,
                                                     _plot_tab->currentIndex()==PlotIntegral ?
                                                     _integral_plot->expression() :
                                                     _contrast_plot->expression()));
                                     
  delete desc;

  _ovls.push_back(ovl);
  
  emit changed();
}

void ImageRPhiProjection::remove_overlay(QtOverlay* obj)
{
  CursorOverlay* ovl = static_cast<CursorOverlay*>(obj);
  _ovls.remove(ovl);
  
  //  emit changed();
}
