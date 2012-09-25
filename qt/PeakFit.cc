#include "PeakFit.hh"

#if 0
#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescScan.hh"
#include "ami/qt/DescChart.hh"
#else
#include "ami/qt/ScalarPlotDesc.hh"
#endif
#include "ami/qt/AxisInfo.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/EdgeCursor.hh"
#include "ami/qt/PeakFitPlot.hh"
#include "ami/qt/PeakFitPost.hh"
#include "ami/qt/PeakFitOverlay.hh"
#include "ami/qt/WaveformDisplay.hh"
#include "ami/qt/Calculator.hh"
#include "ami/qt/PlotFrame.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/CursorsX.hh"
#include "ami/qt/CursorDefinition.hh"
#include "ami/qt/PostAnalysis.hh"
#include "ami/qt/QtPlotSelector.hh"

#if 0
#include "ami/data/DescScalar.hh"
#include "ami/data/DescScalarRange.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescScan.hh"
#else
#endif
#include "ami/data/DescCache.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/PeakFitPlot.hh"

#include "ami/data/Integral.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QComboBox>
#include <QtGui/QDoubleSpinBox>
#include <QtCore/QRegExp>
#include <QtGui/QRegExpValidator>
#include <QtGui/QMessageBox>
#include <QtGui/QTabWidget>

#include <sys/socket.h>

// works for labels not buttons
//#define bold(t) "<b>" #t "</b>"
#define bold(t) #t

enum { ConstantBL, LinearBL };
enum { _TH1F, _vT, _vF, _vS };


using namespace Ami::Qt;

PeakFit::PeakFit(QWidget* parent, ChannelDefinition* channels[], unsigned nchannels, 
                 WaveformDisplay& frame, QtPWidget* frameParent) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _frameParent(frameParent),
  _clayout  (new QVBoxLayout)
{
  _names << "a" << "b" << "c" << "d" << "f" << "g" << "h" << "i" << "j" << "k";

  setWindowTitle("PeakFit Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _baseline  = new EdgeCursor(QString(""), *_frame.plot(), frameParent);
  QString bl("baseline");
  _baseline->setName(bl);

  QStringList q;
  for(unsigned k=0; k<Ami::PeakFitPlot::NumberOf; k++)
    q << Ami::PeakFitPlot::name((Ami::PeakFitPlot::Parameter)k);
  QComboBox* qtyBox = new QComboBox;
  qtyBox->addItems(q);

  _title    = new QLineEdit("Peak plot");

#if 0
  QPushButton* addPostB = new QPushButton("Post");

  _hist   = new DescTH1F (bold(Sum (1dH)));
  _vTime  = new DescChart(bold(Mean v Time));
  _vFeature = new DescProf (bold(Mean v Var) , &FeatureRegistry::instance());
  _vScan    = new DescScan (bold(Mean v Scan));
#else
  _scalar_desc = new ScalarPlotDesc(0,&FeatureRegistry::instance(),false);
#endif

  _lvalue = new CursorLocation;
  QPushButton *lgrabB  = new QPushButton("Grab");

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* ovlyB  = new QPushButton("Overlay");
  QPushButton* closeB = new QPushButton("Close");
  
  QVBoxLayout* layout = new QVBoxLayout;
  { QLabel* desc = new QLabel;
    desc->setWordWrap(true);
    desc->setAlignment(::Qt::AlignHCenter);
    desc->setText("Finds position, height, and width of largest peak from 'baseline'.");
    layout->addWidget(desc); }
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* locations_box = new QGroupBox("Define PeakFit");
    locations_box->setToolTip("Define baseline value.");
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Baseline"));
      { _baseline_tab = new QTabWidget;
        _baseline_tab->addTab(_baseline, "Constant");
        { QVBoxLayout* vl = new QVBoxLayout;
          { QHBoxLayout* hl = new QHBoxLayout;
            hl->addWidget(_lvalue);
            hl->addWidget(lgrabB);
            vl->addLayout(hl); }
          vl->addLayout(_clayout);
          QWidget* lw = new QWidget;
          lw->setLayout(vl);
          _baseline_tab->addTab(lw, "Linear"); }
        layout2->addWidget(_baseline_tab); }
      layout1->addLayout(layout2); }
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Quantity"));
      layout2->addWidget(qtyBox);
      layout1->addLayout(layout2); }
    locations_box->setLayout(layout1);
    layout->addWidget(locations_box); }
#if 0
  { QHBoxLayout* layout2 = new QHBoxLayout;
    layout2->addWidget(new QLabel("Title"));
    layout2->addWidget(_title);
    layout2->addStretch();
    layout2->addWidget(addPostB);
    layout->addLayout(layout2); }
  { QGroupBox* plot_box = new QGroupBox("Plot Type");
    QVBoxLayout* layout1 = new QVBoxLayout;
    _plottype_tab = new QTabWidget;
    _plottype_tab->addTab(_hist    , _hist    ->button()->text());
    _plottype_tab->addTab(_vTime   , _vTime   ->button()->text());
    _plottype_tab->addTab(_vFeature, _vFeature->button()->text());
    _plottype_tab->addTab(_vScan   , _vScan   ->button()->text());
    layout1->addWidget(_plottype_tab);
    plot_box->setLayout(layout1); 
    layout->addWidget(plot_box); }
#else
  { QHBoxLayout* layout2 = new QHBoxLayout;
    layout2->addWidget(new QLabel("Title"));
    layout2->addWidget(_title);
    layout2->addStretch();
    layout->addLayout(layout2); }
  layout->addWidget(_scalar_desc);
#endif
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(ovlyB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(qtyBox    , SIGNAL(activated(int)), this, SLOT(set_quantity(int)));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));

  connect(lgrabB    , SIGNAL(clicked()),      this, SLOT(grab_cursorx()));
  connect(_lvalue   , SIGNAL(returnPressed()),this, SLOT(add_cursor()));
  connect(this      , SIGNAL(grabbed()),      this, SLOT(add_cursor()));
  connect(this      , SIGNAL(grabbed()),      this, SLOT(front()));

#if 0
  connect(addPostB  , SIGNAL(clicked()),      this, SLOT(add_post()));
#else
  _scalar_desc->post(this, SLOT(add_post()));
#endif
  connect(_baseline , SIGNAL(changed()),      this, SLOT(front()));

  set_quantity(0);
}
  
PeakFit::~PeakFit()
{
  for(std::list<PeakFitPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
}

void PeakFit::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert(p, "QLineEdit", "_baseline", QtPersistent::insert(p,_baseline ->value()) );
  XML_insert(p, "QLineEdit", "_title", QtPersistent::insert(p,_title->text()) );
#if 0
  XML_insert(p, "DescTH1F", "_hist", _hist->save(p) );
  XML_insert(p, "DescChart", "_vTime", _vTime->save(p) );
  XML_insert(p, "DescProf", "_vFeature", _vFeature->save(p) );
  XML_insert(p, "DescScan", "_vScan", _vScan->save(p) );
  XML_insert(p, "QTabWidget", "_plottype_tab", QtPersistent::insert(p,_plottype_tab->currentIndex()));
#else
  XML_insert(p, "ScalarPlotDesc", "_scalar_desc", _scalar_desc->save(p) );
#endif
  XML_insert(p, "QTabWidget", "_baseline_tab", QtPersistent::insert(p,_baseline_tab->currentIndex()));

  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++) {
    XML_insert(p, "CursorDefinition", "_cursors", (*it)->save(p) );
  }

  for(std::list<PeakFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert(p, "PeakFitPlot", "_plots", (*it)->save(p) );
  }

  for(std::list<PeakFitPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    XML_insert(p, "PeakFitPost", "_posts", (*it)->save(p) );
  }
  for(std::list<PeakFitOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++) {
    XML_insert(p, "PeakFitOverlay", "_ovls", (*it)->save(p) );
  }
}

void PeakFit::load(const char*& p)
{
  for(std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end(); it++) {
    _names.push_back((*it)->name());
    delete *it;
  }
  _cursors.clear();

  for(std::list<PeakFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
    delete *it;
  }
  _plots.clear();

  for(std::list<PeakFitPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  _ovls .clear();

  XML_iterate_open(p,tag)
    if (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_baseline")
      _baseline->value(QtPersistent::extract_d(p));
    else if (tag.name == "_title")
      _title->setText(QtPersistent::extract_s(p));
#if 0
    else if (tag.name == "_hist")
      _hist->load(p);
    else if (tag.name == "_vTime")
      _vTime->load(p);
    else if (tag.name == "_vFeature")
      _vFeature->load(p);
    else if (tag.name == "_vScan")
      _vScan->load(p);
    else if (tag.name == "_plottype_tab")
      _plottype_tab->setCurrentIndex(QtPersistent::extract_i(p));
#else
    else if (tag.name == "_scalar_desc")
      _scalar_desc->load(p);
#endif
    else if (tag.name == "_baseline_tab")
      _baseline_tab->setCurrentIndex(QtPersistent::extract_i(p));
    else if (tag.name == "_cursors") {
      CursorDefinition* d = new CursorDefinition(p, *this, _frame.plot());
      _cursors.push_back(d);
      _clayout->addWidget(d);
      _names.removeAll(d->name());
    }    
    else if (tag.name == "_plots") {
      PeakFitPlot* plot = new PeakFitPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_posts") {
      PeakFitPost* post = new PeakFitPost(p);
      _posts.push_back(post);
    }
    else if (tag.name == "_ovls") {
      PeakFitOverlay* ovl = new PeakFitOverlay(*this, p);
      _ovls.push_back(ovl);
    }
  XML_iterate_close(PeakFit,tag);
}

void PeakFit::save_plots(const QString& p) const
{
  int i=1;
  for(std::list<PeakFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg(i++);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
}

void PeakFit::configure(char*& p, unsigned input, unsigned& output,
			 ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			 ConfigureRequest::Source source)
{
  for(std::list<PeakFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo(),source);
  for(std::list<PeakFitPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo(),source);
  for(std::list<PeakFitOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo(),source);
}

void PeakFit::setup_payload(Cds& cds)
{
  for(std::list<PeakFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<PeakFitOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->setup_payload(cds);
}

void PeakFit::update()
{
  for(std::list<PeakFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  for(std::list<PeakFitOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
}

void PeakFit::set_channel(int c) 
{ 
  _channel=c; 
}

void PeakFit::set_quantity(int q)
{
  _quantity=q;
}

void PeakFit::plot()
{
  if (_baseline_tab->currentIndex()!=ConstantBL && !_cursors.size())  /* Can't plot if no baseline! */
      return;

  if (_scalar_desc->postAnalysis()) {
    QString     qtitle = _add_post();
    DescEntry*  entry  = _scalar_desc->desc(qPrintable(qtitle));
    PostAnalysis::instance()->plot(qtitle,entry,_posts.back());
  }
  else {
    QString qtitle = QString("%1_%2")
      .arg(_title->text())
      .arg(Ami::PeakFitPlot::name((Ami::PeakFitPlot::Parameter)_quantity));

    DescEntry* desc = _scalar_desc->desc(qPrintable(qtitle));

    Ami::PeakFitPlot *plotter;
    if (_baseline_tab->currentIndex()==ConstantBL) {
      plotter = new Ami::PeakFitPlot(*desc, _baseline->value(), (Ami::PeakFitPlot::Parameter)_quantity);
    } else {
      int bins[MAX_BINS], i = 0;
      for (std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end() && i < MAX_BINS; i++, it++) {
        bins[i] = _frame.xinfo().tick((*it)->location());
      }
      plotter = new Ami::PeakFitPlot(*desc, i, bins, (Ami::PeakFitPlot::Parameter)_quantity);
    }
    PeakFitPlot* plot = new PeakFitPlot(this,
                                        qtitle,
                                        _channel,
                                        plotter);
    
    _plots.push_back(plot);

    connect(plot, SIGNAL(changed()), this, SIGNAL(changed()));
    connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  }
  emit changed();
}

void PeakFit::remove_plot(QObject* obj)
{
  PeakFitPlot* plot = static_cast<PeakFitPlot*>(obj);
  _plots.remove(plot);

  disconnect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

void PeakFit::add_post()
{
  _add_post();
  _posts.back()->signup();
}

QString PeakFit::_add_post()
{
  //
  //  Check that a post variable by the same name doesn't already exist
  //
#if 0
  QString qtitle = FeatureRegistry::instance(Ami::PostAnalysis).
    validate_name(QString("%1")
                  .arg(Ami::PeakFitPlot::name((Ami::PeakFitPlot::Parameter)_quantity)));
#else
  QString qtitle = FeatureRegistry::instance(Ami::PostAnalysis).
    validate_name(QString("%1_%2").arg(_scalar_desc->qtitle())
                  .arg(Ami::PeakFitPlot::name((Ami::PeakFitPlot::Parameter)_quantity)));
#endif

  //
  //  Add to the list of post variables
  //
  Ami::DescCache* desc = new Ami::DescCache(qPrintable(qtitle), 
                                            qPrintable(qtitle),
                                            Ami::PostAnalysis);
  Ami::PeakFitPlot *plotter;
  if (_baseline_tab->currentIndex()==ConstantBL) {
      plotter = new Ami::PeakFitPlot(*desc, _baseline->value(), (Ami::PeakFitPlot::Parameter)_quantity);
  } else {
      int bins[MAX_BINS], i = 0;
      for (std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end() && i < MAX_BINS; i++, it++)
          bins[i] = _frame.xinfo().tick((*it)->location());
      plotter = new Ami::PeakFitPlot(*desc,i, bins, (Ami::PeakFitPlot::Parameter)_quantity);
  }
  PeakFitPost* post = new PeakFitPost(_channel, plotter, this);
							   
  _posts.push_back(post);

  emit changed();

  return qtitle;
}

void PeakFit::grab_cursorx() 
{
  _frame.plot()->set_cursor_input(this); 
  if (_frameParent)
    _frameParent->front();
}

void PeakFit::add_cursor()
{
  if (_names.size()) {
    CursorDefinition* d = new CursorDefinition(_names.takeFirst(),
					       _lvalue->value(),
					       *this,
					       _frame.plot());
    _cursors.push_back(d);
    _clayout->addWidget(d);
    _baseline_tab->updateGeometry();
  }
  else {
    QMessageBox::critical(this,tr("Add Cursor"),tr("Too many cursors in use"));
  }
}

void PeakFit::remove(CursorDefinition& c)
{
  _names.push_back(c.name());
  _cursors.remove(&c);
  delete &c;
  _baseline_tab->updateGeometry();
}

void PeakFit::mousePressEvent(double x, double y)
{
  _frame.plot()->set_cursor_input(0);
  _lvalue->setText(QString::number(x));
  emit grabbed();
}

void PeakFit::mouseMoveEvent   (double,double) {}
void PeakFit::mouseReleaseEvent(double,double) {}

void PeakFit::remove_peakfit_post(PeakFitPost* post)
{
  _posts.remove(post);
  emit changed();
}

void PeakFit::overlay()
{
  if (_baseline_tab->currentIndex()!=ConstantBL && !_cursors.size())  /* Can't plot if no baseline! */
      return;

  DescEntry* desc;
  if (_scalar_desc->postAnalysis()) {
    QString     qtitle = _add_post();
    desc  = _scalar_desc->desc(qPrintable(qtitle));
    new QtPlotSelector(*this, *PostAnalysis::instance(), desc, _posts.back());
  }
  else {
    QString qtitle = QString("%1_%2")
      .arg(_title->text())
      .arg(Ami::PeakFitPlot::name((Ami::PeakFitPlot::Parameter)_quantity));

    DescEntry* desc = _scalar_desc->desc(qPrintable(qtitle));
    new QtPlotSelector(*this, *this, desc);
  }
}

void PeakFit::add_overlay(DescEntry* desc, QtPlot* plot, SharedData*)
{
  Ami::PeakFitPlot *plotter;
  if (_baseline_tab->currentIndex()==ConstantBL) {
    plotter = new Ami::PeakFitPlot(*desc, _baseline->value(), 
                                   (Ami::PeakFitPlot::Parameter)_quantity);
  } else {
    int bins[MAX_BINS], i = 0;
    for (std::list<CursorDefinition*>::const_iterator it=_cursors.begin(); it!=_cursors.end() && i < MAX_BINS; i++, it++) {
      bins[i] = _frame.xinfo().tick((*it)->location());
    }
    plotter = new Ami::PeakFitPlot(*desc, i, bins, (Ami::PeakFitPlot::Parameter)_quantity);
  }

  PeakFitOverlay* ovl = new PeakFitOverlay(*this,
                                           *plot,
                                           _channel,
                                           plotter);
    
  _ovls.push_back(ovl);

  emit changed();
}

void PeakFit::remove_overlay(QtOverlay* o)
{
  PeakFitOverlay* ovl = static_cast<PeakFitOverlay*>(o);

  _ovls.remove(ovl);

  //  emit changed();
}
