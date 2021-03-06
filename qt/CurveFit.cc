#include "CurveFit.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/CursorDefinition.hh"
#include "ami/qt/WaveformDisplay.hh"
#include "ami/qt/PlotFrame.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/CurveFitPlot.hh"
#include "ami/qt/CurveFitPost.hh"
#include "ami/qt/CurveFitOverlay.hh"
#include "ami/qt/PostAnalysis.hh"
#include "ami/qt/QtPlotSelector.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescWaveform.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/Entry.hh"
#include "ami/data/Reference.hh"
#include "ami/data/CurveFit.hh"

#include <QtGui/QFileDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QCheckBox>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>

#include <sys/socket.h>

using namespace Ami::Qt;

static const char* opname[] = {
    "scale",
    "shift",
    "chi squared"
};

CurveFit::CurveFit(QWidget* parent,
		       ChannelDefinition* channels[], unsigned nchannels, WaveformDisplay& frame) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _file     (new QLabel("No reference")),
  _outBox   (new QComboBox),
  _list_sem (Semaphore::FULL)
{
  _fname.clear();

  setWindowTitle("CurveFit Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _outBox->addItem("scale");
  _outBox->addItem("shift");
  _outBox->addItem("chi-squared");
  _outBox->setCurrentIndex(0);

  _scalar_desc = new ScalarPlotDesc(0);

  QPushButton* loadB  = new QPushButton("Load");
  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* ovlyB  = new QPushButton("Overlay");
  QPushButton* closeB  = new QPushButton("Close");

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* ref_box = new QGroupBox("Reference Waveform");
    QVBoxLayout* layout1 = new QVBoxLayout;
    QHBoxLayout* layout2 = new QHBoxLayout;
    layout2->addWidget(new QLabel("Reference File:"));
    layout2->addWidget(loadB);
    layout1->addLayout(layout2);
    layout1->addWidget(_file);
    ref_box->setLayout(layout1);
    layout->addWidget(ref_box); }
  { QGroupBox* out_box = new QGroupBox("Output");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Output"));
    layout1->addWidget(_outBox);
    layout1->addStretch();
    out_box->setLayout(layout1);
    layout->addWidget(out_box); }
  { layout->addWidget(_scalar_desc); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(ovlyB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  layout->addStretch();
  setLayout(layout);

  connect(loadB     , SIGNAL(clicked()),      this, SLOT(load_file()));
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(closeB    , SIGNAL(clicked()),      this, SIGNAL(closed()));

  _scalar_desc->post(this, SLOT(add_post()));
}
  
CurveFit::~CurveFit()
{
  _list_sem.take();
  for(std::list<CurveFitPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  _list_sem.give();
}

void CurveFit::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );
  XML_insert(p, "ScalarPlotDesc", "_scalar_desc", _scalar_desc->save(p) );
  XML_insert(p, "QString", "_fname", QtPersistent::insert(p, _fname));

  for(std::list<CurveFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert(p, "CurveFitPlot", "_plots", (*it)->save(p) );
  }
  for(std::list<CurveFitPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    XML_insert(p, "CurveFitPost", "_posts", (*it)->save(p) );
  }
  for(std::list<CurveFitOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++) {
    XML_insert(p, "CurveFitOverlay", "_ovls", (*it)->save(p) );
  }
}

void CurveFit::load(const char*& p)
{
  _list_sem.take();

  for(std::list<CurveFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _plots.clear();

  for(std::list<CurveFitPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  _ovls .clear();

  XML_iterate_open(p,tag)

    if (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_scalar_desc")
      _scalar_desc->load(p);
    else if (tag.name == "_fname") {
      _fname = QtPersistent::extract_s(p);
      if (_fname.isNull())
          _file->setText("No reference");
      else
          _file->setText("Set: " + _fname);
    } else if (tag.name == "_plots") {
      CurveFitPlot* plot = new CurveFitPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_posts") {
      CurveFitPost* post = new CurveFitPost(p);
      _posts.push_back(post);
    }
    else if (tag.name == "_ovls") {
      CurveFitOverlay* ovl = new CurveFitOverlay(*this, p);
      _ovls.push_back(ovl);
    }
  XML_iterate_close(CurveFit,tag);

  _list_sem.give();

  emit changed();
}

void CurveFit::snapshot(const QString& p) const
{
  for(std::list<CurveFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg((*it)->_name));
}

void CurveFit::save_plots(const QString& p) const
{
  for(std::list<CurveFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg((*it)->_name);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
}

void CurveFit::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  _list_sem.take();
  for(std::list<CurveFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo());
  for(std::list<CurveFitPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo(),ConfigureRequest::Analysis);
  for(std::list<CurveFitOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo());
  _list_sem.give();
}

void CurveFit::setup_payload(Cds& cds)
{
  _list_sem.take();
  for(std::list<CurveFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<CurveFitOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->setup_payload(cds);
  _list_sem.give();
}

void CurveFit::update()
{
  _list_sem.take();
  for(std::list<CurveFitPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  for(std::list<CurveFitOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
  _list_sem.give();
}

void CurveFit::initialize(const Ami::DescEntry& e)
{
}

void CurveFit::load_file()
{
  _fname = QFileDialog::getOpenFileName(this, "Load Reference Waveform from File",
                                        Path::base(), "*.dat;;*.ref");
  if (_fname.isNull()) {
      _file->setText("No reference");
      printf("load_file file is null\n");
      return;
  } else
      _file->setText("Set: " + _fname);
}

void CurveFit::set_channel(int c) 
{ 
  _channel=c; 
}

void CurveFit::plot()
{
  if (_fname.isNull())
    return;

  if (_scalar_desc->postAnalysis()) {
    QString     qtitle = _add_post();
    DescEntry*  entry  = _scalar_desc->desc(qPrintable(qtitle));
    PostAnalysis::instance()->plot(qtitle,entry,_posts.back());
  }
  else {
    int op = _outBox->currentIndex();
    DescEntry *desc = _scalar_desc->desc(opname[op]);
    QString norm(_scalar_desc->expr(QString(""))); /* This has the form "()/(NORMVAR)" or "" */
  
    if (norm.length()) {
      norm = norm.mid(4, norm.length() - 5); // Just the name, please!
    }

    CurveFitPlot* plot = new CurveFitPlot(this, desc->name(), _channel,
                                          new Ami::CurveFit(qPrintable(_fname),
                                                            op, *desc, qPrintable(norm)));

    _list_sem.take();
    _plots.push_back(plot);
    _list_sem.give();

    connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  }
  emit changed();
}

void CurveFit::remove_plot(QObject* obj)
{
  CurveFitPlot* plot = static_cast<CurveFitPlot*>(obj);
  _list_sem.take();
  _plots.remove(plot);
  _list_sem.give();

  disconnect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  delete plot;
}

void CurveFit::add_post()
{
  if (_fname.isNull())
    return;

  _add_post();
  _posts.back()->signup();
}

QString CurveFit::_add_post()
{
    QString qtitle = FeatureRegistry::instance(Ami::PostAnalysis).validate_name(_scalar_desc->qtitle());

    Ami::DescCache* desc = new Ami::DescCache(qPrintable(qtitle),
                                              qPrintable(qtitle),
                                              Ami::PostAnalysis);
    Ami::CurveFit* fit = new Ami::CurveFit(qPrintable(_fname),
                                           _outBox->currentIndex(), *desc);
    CurveFitPost* post = new CurveFitPost(_channel, fit, this);
    _list_sem.take();
    _posts.push_back(post);
    _list_sem.give();

    delete desc;

    emit changed();

    FeatureRegistry::instance(Ami::PostAnalysis).share(qtitle,post);

    return qtitle;
}

void CurveFit::remove_curvefit_post(CurveFitPost* post)
{
  _list_sem.take();
  _posts.remove(post);
  _list_sem.give();

  delete post;
  emit changed();
}

void CurveFit::overlay()
{
  if (_fname.isNull())
    return;

  if (_scalar_desc->postAnalysis()) {
    QString     qtitle = _add_post();
    DescEntry*  entry  = _scalar_desc->desc(qPrintable(qtitle));
    new QtPlotSelector(*this, *PostAnalysis::instance(), entry, _posts.back());
  }
  else {
    int op = _outBox->currentIndex();
    DescEntry *desc = _scalar_desc->desc(opname[op]);

    new QtPlotSelector(*this, *this, desc);
  }
}

void CurveFit::add_overlay(DescEntry* desc,
                           QtPlot*    plot,  
                           SharedData*)
{
  QString norm(_scalar_desc->expr(QString(""))); /* This has the form "()/(NORMVAR)" or "" */
  
  if (norm.length()) {
    norm = norm.mid(4, norm.length() - 5); // Just the name, please!
  }

  int op = _outBox->currentIndex();
  CurveFitOverlay* ovl = new CurveFitOverlay(*this, *plot,
                                             _channel,
                                             new Ami::CurveFit(qPrintable(_fname),
                                                               op, *desc, qPrintable(norm)));

  _list_sem.take();
  _ovls.push_back(ovl);
  _list_sem.give();

  emit changed();
}

void CurveFit::remove_overlay(QtOverlay* obj)
{
  CurveFitOverlay* ovl = static_cast<CurveFitOverlay*>(obj);
  _list_sem.take();
  _ovls.remove(ovl);
  _list_sem.give();
  
  //  emit changed();
}
