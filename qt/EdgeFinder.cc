#include "EdgeFinder.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/SMPRegistry.hh"
#include "ami/qt/SMPWarning.hh"
#include "ami/qt/ControlLog.hh"
#include "ami/qt/VectorArrayDesc.hh"

#include "ami/qt/QtPlotSelector.hh"

#include "ami/data/DescWaveform.hh"
#include "ami/data/EdgeFinder.hh"
#include "ami/data/BinMath.hh"
#include "ami/data/RawFilter.hh"
#include "ami/data/Entry.hh"
#include "ami/data/Cds.hh"

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
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>
#include <QtGui/QCheckBox>

#include <sys/socket.h>
#include <stdio.h>

//#define DBUG

static inline int avgRound(int n, int d)
{
  return (n+d-1)/d;
}

using namespace Ami::Qt;

EdgeFinder::EdgeFinder(QWidget* parent,
		       ChannelDefinition* channels[], unsigned nchannels) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0)
{
  _config = new EdgeFinderConfig(this);

  setWindowTitle("Edge Finder");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    _channelBox->addItem(channels[i]->name());

  _setButton = new QPushButton("New");
  _setBox    = new QComboBox;
  new_set();

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* ovlyB  = new QPushButton("Overlay");
  QPushButton* closeB = new QPushButton("Close");

  { QStringList parms;
    parms << Ami::Edges::name(Ami::Edges::Parameter(Ami::Edges::NumberOf));
    for(unsigned i=0; i<Ami::Edges::NumberOf; i++)
      parms << Ami::Edges::name(Ami::Edges::Parameter(i));
    _analysis_plot = new VectorArrayDesc(0,parms); }
  _title         = new QLineEdit("Edges");

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox;
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Source Channel"));
    layout1->addWidget(_channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* selection_box = new QGroupBox("Edge Selection");
    selection_box->setToolTip("Define edge selection criteria.");
    QVBoxLayout* layout2 = new QVBoxLayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addStretch();
      layout1->addWidget(new QLabel("Select"));
      layout1->addWidget(_setBox);
      layout1->addWidget(_setButton);
      layout1->addStretch();
      layout2->addLayout(layout1); }
    layout2->addWidget(_config);
    selection_box->setLayout(layout2);
    layout->addWidget(selection_box); }

  { QGroupBox* plot_box = new QGroupBox;
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Plot Title"));
      layout2->addWidget(_title);
      layout1->addLayout(layout2); }
    layout1->addWidget(_analysis_plot);
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

  connect(_channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(_setButton , SIGNAL(clicked()),      this, SLOT(new_set()));
  connect(plotB      , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB      , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(closeB     , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(_channelBox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_channel()));
  for(unsigned i=0; i<_nchannels; i++)
    connect(_channels[i], SIGNAL(agg_changed()), this, SLOT(change_channel()));
  connect(_config    , SIGNAL(changed()),      this, SLOT(update_config()));
}
  
EdgeFinder::~EdgeFinder()
{
}

void EdgeFinder::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );
  XML_insert(p, "int", "_channel", QtPersistent::insert(p,_channel) );
  XML_insert(p, "EdgeFinderConfig", "_config", _config->save(p) );

  for(unsigned i=0; i<_configs.size(); i++) {
    XML_insert(p, "EdgeFinderConfig", "_configs", _configs[i]->save(p));
  }

  for(unsigned i=0; i<_apps.size(); i++) {
    XML_insert(p, "VAConfigApp", "_apps", _apps[i]->save(p));
  }
}

void EdgeFinder::load(const char*& p)
{
  for(unsigned i=0; i<_configs.size(); i++)
    delete _configs[i];
  _configs.clear();

  for(unsigned i=0; i<_apps.size(); i++)
    delete _apps[i];
  _apps.clear();

  _setBox->clear();


  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_config")
      _config->load(p);
    else if (tag.name == "_configs") {
      Ami::EdgeFinderConfig* c = new Ami::EdgeFinderConfig;
      c->load(p); 
      _configs.push_back(c);
    }
    else if (tag.name == "_apps") {
      int set = _apps.size()/_nchannels;
      QString name = QString("Set%1").arg(set);
      unsigned channel = _apps.size()%_nchannels;
      if (channel==0)
        _setBox->addItem(name);
      EdgeFinderConfigApp* app = new EdgeFinderConfigApp(this, name, channel, *_configs[set]);
      app->load(p);
      _apps.push_back(app);
      connect(app, SIGNAL(changed()), this, SIGNAL(changed()));
    }
  XML_iterate_close(Droplet,tag);
}

void EdgeFinder::save_plots(const QString& p) const
{
  for(unsigned i=0; i<_apps.size(); i++) {
    QString q = QString("%1_Set%2_%3").arg(p).arg(i/4).arg(i%4);
    _apps[i]->save_plots(q);
  }
}

void EdgeFinder::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  for(unsigned i=0; i<_apps.size(); i++)
    _apps[i]->configure(p,input,output,channels,signatures,nchannels);
}

void EdgeFinder::setup_payload(Cds& cds)
{
  for(unsigned i=0; i<_apps.size(); i++)
    _apps[i]->setup_payload(cds);
}

void EdgeFinder::update()
{
  for(unsigned i=0; i<_apps.size(); i++)
    _apps[i]->update();
}

void EdgeFinder::set_channel(int c) 
{ 
  _channel=c; 
}

void EdgeFinder::update_config()
{
  *_configs[_setBox->currentIndex()] = _config->value();
  emit changed();
}

void EdgeFinder::plot()
{
  if (_analysis_plot->postAnalysis()) {
#if 0
    SharedData* post;
    QString qtitle = _app().add_post(pplot->qtitle(),
				     _analysis_plot->expression(),
				     post);
    DescEntry*  entry  = pplot->desc(qPrintable(qtitle));
    PostAnalysis::instance()->plot(qtitle,entry,post);
#endif
  }
  else {
    QString title = QString("%1 [%2]").arg(_channels[_channel]->name()).arg(_analysis_plot->title());
    DescEntry*  desc = _analysis_plot->desc(qPrintable(title));
    _app().add_cursor_plot(new BinMath(*desc,
				       _analysis_plot->expression()));
    delete desc;
  }
}

void EdgeFinder::overlay()
{
  if (_analysis_plot->postAnalysis()) {
#if 0
    SharedData* post;
    QString qtitle = _app().add_post(pplot->qtitle(),
				     _analysis_plot->expression(),
				     post);
    DescEntry*  entry  = pplot->desc(qPrintable(qtitle));
    PostAnalysis::instance()->plot(qtitle,entry,post);
#endif
  }
  else {
    QString title = QString("%1 [%2]").arg(_channels[_channel]->name()).arg(_analysis_plot->title());
    DescEntry*  desc = _analysis_plot->desc(qPrintable(title));
    new QtPlotSelector(*this, *this, desc);
  }
}

void EdgeFinder::add_overlay   (DescEntry* desc, QtPlot* plot, SharedData*) 
{
  _app().add_overlay(*plot, new BinMath(*desc,
					_analysis_plot->expression()));
  delete desc;
}

void EdgeFinder::remove_overlay(QtOverlay*) {}
void EdgeFinder::update_interval() {}
void EdgeFinder::change_channel() 
{
  int ich = _channelBox->currentIndex();
  _channel=ich; 
}

void EdgeFinder::prototype(const DescEntry& i)
{
}

void EdgeFinder::new_set()
{
  int i=_apps.size()/_nchannels;
  _configs.push_back(new Ami::EdgeFinderConfig);
  QString name = QString("Set%1").arg(i);
  _setBox->addItem(name);
  for(unsigned j=0; j<_nchannels; j++) {
    EdgeFinderConfigApp* set = new EdgeFinderConfigApp(this, name,j,*_configs[i]);
    _apps.push_back(set);
    connect(set, SIGNAL(changed()), this, SIGNAL(changed()));
  }
  _setBox->setCurrentIndex(i);
}

void EdgeFinder::select_set(int i)
{
  if (i>=0)
    _config->load(*_configs[i]);
}

EdgeFinderConfigApp& EdgeFinder::_app() { return *_apps[_setBox->currentIndex()*_nchannels + _channelBox->currentIndex()]; }


EdgeFinderConfigApp::EdgeFinderConfigApp(QWidget* parent, 
					 const QString& name, 
					 unsigned i, 
					 const Ami::EdgeFinderConfig& c) :
  VAConfigApp(parent,name,i),
  _config    (c)
{
}

EdgeFinderConfigApp::~EdgeFinderConfigApp() 
{
}

Ami::AbsOperator* EdgeFinderConfigApp::_op(const char* name)
{
  return new Ami::EdgeFinder(name,_config);
}


EdgeFinderConfig::EdgeFinderConfig(QWidget* parent) : QWidget(parent)
{ QGridLayout* l = new QGridLayout;
  unsigned row=0;
  l->addWidget(new QLabel("Fraction"),row,0,::Qt::AlignRight);
  l->addWidget(_fraction = new QLineEdit("0.5")    ,row,1,::Qt::AlignLeft);
  new QDoubleValidator(_fraction); row++;
  connect(_fraction, SIGNAL(editingFinished()), this, SIGNAL(changed()));

  l->addWidget(_leading_edge = new QCheckBox("Leading Edges"),row,0,1,2,::Qt::AlignCenter);
  _leading_edge->setChecked(true);
  row++;
  connect(_leading_edge, SIGNAL(toggled()), this, SIGNAL(changed()));
	
  l->addWidget(new QLabel("Deadtime [sec]"),row,0,::Qt::AlignRight);
  l->addWidget(_deadtime = new QLineEdit("0")  ,row,1,::Qt::AlignLeft);
  new QDoubleValidator(_deadtime); row++;
  connect(_deadtime, SIGNAL(editingFinished()), this, SIGNAL(changed()));
	
  l->addWidget(new QLabel("Threshold"),row,0,::Qt::AlignRight);
  l->addWidget(_threshold_value = new QLineEdit("0")  ,row,1,::Qt::AlignLeft);
  new QDoubleValidator(_threshold_value); row++;
  connect(_threshold_value, SIGNAL(editingFinished()), this, SIGNAL(changed()));
	
  l->addWidget(new QLabel("Baseline")  ,row,0,::Qt::AlignRight);
  l->addWidget(_baseline_value = new QLineEdit("0"),row,1,::Qt::AlignLeft);
  new QDoubleValidator(_baseline_value); row++;
  connect(_baseline_value, SIGNAL(editingFinished()), this, SIGNAL(changed()));

  setLayout(l);
}

EdgeFinderConfig::~EdgeFinderConfig() {}

Ami::EdgeFinderConfig EdgeFinderConfig::value() const {
  Ami::EdgeFinderConfig v;
  v._fraction        = _fraction->text().toDouble();
  v._leading_edge    = _leading_edge->isChecked();
  v._deadtime        = _deadtime->text().toDouble();
  v._threshold_value = _threshold_value->text().toDouble();
  v._baseline_value  = _baseline_value ->text().toDouble();
  return v;
}

void EdgeFinderConfig::load(const Ami::EdgeFinderConfig& v) {
  _fraction       ->setText(QString::number(v._fraction));
  _leading_edge   ->setChecked(v._leading_edge);
  _deadtime       ->setText(QString::number(v._deadtime));
  _threshold_value->setText(QString::number(v._threshold_value));
  _baseline_value ->setText(QString::number(v._baseline_value));
}

void EdgeFinderConfig::load(const char*& p) {
  XML_iterate_open(p,tag)
    if (tag.name == "_fraction")
      _fraction->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_leading_edge")
      _leading_edge->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_deadtime")
      _deadtime->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_threshold_value")
      _threshold_value->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_baseline_value")
      _baseline_value->setText(QtPersistent::extract_s(p));
  XML_iterate_close(EdgeFinderConfig,tag);
}

void EdgeFinderConfig::save(char*& p) const {
  XML_insert(p, "QLineEdit", "_fraction"      , QtPersistent::insert(p,_fraction->text()) );
  XML_insert(p, "QCheckBox", "_leading_edge"  , QtPersistent::insert(p,_leading_edge->isChecked()) );
  XML_insert(p, "QLineEdit", "_deadtime"      , QtPersistent::insert(p,_deadtime->text()) );
  XML_insert(p, "QLineEdit", "_threshold_value", QtPersistent::insert(p,_threshold_value->text()) );
  XML_insert(p, "QLineEdit", "_baseline_value", QtPersistent::insert(p,_baseline_value->text()) );
}
