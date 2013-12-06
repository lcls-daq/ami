#include "Droplet.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageScale.hh"
#include "ami/qt/SMPRegistry.hh"
#include "ami/qt/SMPWarning.hh"
#include "ami/qt/ControlLog.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/qt/VectorArrayDesc.hh"
#include "ami/qt/QtPlotSelector.hh"

#include "ami/data/DescImage.hh"
#include "ami/data/Droplet.hh"
#include "ami/data/BinMath.hh"
#include "ami/data/VAPlot.hh"
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

namespace Ami {
  namespace Qt {
    class MapPlotDesc : public QWidget {
      enum { Count, Sum };
    public:
      MapPlotDesc(QWidget* p) : QWidget(p) 
      { 
	_accumulate = new QCheckBox("accumulate events");
	_accumulate->setChecked(true);

	_interval  = new QLineEdit;
	_intervalq = new QLabel;
	new QIntValidator(_interval);
  
	_smp_warning = new SMPWarning;

	_proc_grp = new QButtonGroup;
	QRadioButton* countB = new QRadioButton("count hits");
	QRadioButton* sumB   = new QRadioButton("sum values");
	_proc_grp->addButton(countB, Count);
	_proc_grp->addButton(sumB  , Sum);
	countB->setChecked(true);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addStretch();
	{ QHBoxLayout* layout1 = new QHBoxLayout;
	  layout1->addStretch();
	  layout1->addWidget(countB);
	  layout1->addStretch();
	  layout1->addWidget(sumB); 
	  layout1->addStretch();
	  layout->addLayout(layout1); }
	{ QHBoxLayout* layout1 = new QHBoxLayout;
	  layout1->addStretch();
	  layout1->addWidget(_accumulate);
	  layout1->addWidget(_interval);
	  layout1->addWidget(_intervalq);
	  layout1->addWidget(_smp_warning);
	  layout1->addStretch();
	  layout->addLayout(layout1); }
	layout->addStretch();
	setLayout(layout);
      }
      virtual ~MapPlotDesc() {}
    public:
      Ami::AbsOperator* op(const DescImage& o) {
	unsigned nproc = SMPRegistry::instance().nservers();
	return new VAPlot(Ami::Droplets::X,
			  Ami::Droplets::Y,
			  _proc_grp->checkedId()==0 ? -1 :
			  Ami::Droplets::Esum,
			  _accumulate->isChecked() ? 
			  avgRound(_interval->text().toInt(),nproc) : -1,
			  o);
      }
    private:
      QCheckBox* _accumulate;
      QLineEdit* _interval;
      QLabel*    _intervalq;
      SMPWarning* _smp_warning;
      QButtonGroup* _proc_grp;
    };  
  };
};


using namespace Ami::Qt;

enum { PlotMap, PlotAnalysis };

Droplet::Droplet(QWidget* parent,
		 ChannelDefinition* channels[], unsigned nchannels) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _prototype("",0,0)
{
  _config = new DropletConfig(this);

  setWindowTitle("Droplet Analysis");
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

  _plot_tab      = new QTabWidget(0);
  _map_plot      = new MapPlotDesc(0);
  { QStringList parms;
    parms << Ami::Droplets::name(Ami::Droplets::Parameter(Ami::Droplets::NumberOf));
    for(unsigned i=0; i<Ami::Droplets::NumberOf; i++)
      parms << Ami::Droplets::name(Ami::Droplets::Parameter(i));
    _analysis_plot = new VectorArrayDesc(0,parms); }
  _plot_tab->insertTab(PlotMap     ,_map_plot     ,"Map");
  _plot_tab->insertTab(PlotAnalysis,_analysis_plot,"Analysis");
  _title         = new QLineEdit("Droplets");

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox;
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Source Channel"));
    layout1->addWidget(_channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* selection_box = new QGroupBox("Photon Selection");
    selection_box->setToolTip("Define photon selection criteria.");
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

  connect(_channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(_setButton , SIGNAL(clicked()),      this, SLOT(new_set()));
  connect(plotB      , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB     , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(_channelBox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_channel()));
  for(unsigned i=0; i<_nchannels; i++)
    connect(_channels[i], SIGNAL(agg_changed()), this, SLOT(change_channel()));
  connect(_config    , SIGNAL(changed()),      this, SLOT(update_config()));
}
  
Droplet::~Droplet()
{
}

void Droplet::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );
  XML_insert(p, "int", "_channel", QtPersistent::insert(p,_channel) );
  XML_insert(p, "DropletConfig", "_config", _config->save(p) );

  for(unsigned i=0; i<_configs.size(); i++) {
    XML_insert(p, "DropletConfig", "_configs", _configs[i]->save(p));
  }

  for(unsigned i=0; i<_apps.size(); i++) {
    XML_insert(p, "ConfigApp", "_apps", _apps[i]->save(p));
  }
}

void Droplet::load(const char*& p)
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
      Ami::DropletConfig* c = new Ami::DropletConfig;
      c->load(p); 
      _configs.push_back(c);
    }
    else if (tag.name == "_apps") {
      int set = _apps.size()/_nchannels;
      QString name = QString("Set%1").arg(set);
      unsigned channel = _apps.size()%_nchannels;
      if (channel==0)
        _setBox->addItem(name);
      DropletConfigApp* app = new DropletConfigApp(this, name, channel, *_configs[set]);
      app->load(p);
      _apps.push_back(app);
      connect(app, SIGNAL(changed()), this, SIGNAL(changed()));
    }
  XML_iterate_close(Droplet,tag);
}

void Droplet::save_plots(const QString& p) const
{
  for(unsigned i=0; i<_apps.size(); i++) {
    QString q = QString("%1_Set%2_%3").arg(p).arg(i/4).arg(i%4);
    _apps[i]->save_plots(q);
  }
}

void Droplet::configure(char*& p, unsigned input, unsigned& output,
			ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  for(unsigned i=0; i<_apps.size(); i++)
    _apps[i]->configure(p,input,output,channels,signatures,nchannels);
}

void Droplet::setup_payload(Cds& cds)
{
  for(unsigned i=0; i<_apps.size(); i++)
    _apps[i]->setup_payload(cds);
}

void Droplet::update()
{
  for(unsigned i=0; i<_apps.size(); i++)
    _apps[i]->update();
}

void Droplet::set_channel(int c) 
{ 
  _channel=c; 
}

void Droplet::update_config()
{
  *_configs[_setBox->currentIndex()] = _config->value();

#ifdef DBUG
  const Ami::DropletConfig& c = *_configs[_setBox->currentIndex()];
  printf("Droplet::update_config seed %d  nbor %d  esum_min %d max %d  npix_max %d\n",
	 c.seed_threshold,
	 c.nbor_threshold,
	 c.esum_min,
	 c.esum_max,
	 c.npix_max);
#endif

  emit changed();
}

void Droplet::plot()
{
  switch(_plot_tab->currentIndex()) {
  case PlotMap:
    _app().add_map(_map_plot->op(_prototype));
    break;
  case PlotAnalysis:
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
    break;
  default:
    break;
  }
}

void Droplet::overlay()
{
  switch(_plot_tab->currentIndex()) {
  case PlotMap:
    break;
  case PlotAnalysis:
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
    break;
  default:
    break;
  }
}

void Droplet::add_overlay   (DescEntry* desc, QtPlot* plot, SharedData*) 
{
  _app().add_overlay(*plot, new BinMath(*desc,
					_analysis_plot->expression()));
  delete desc;
}

void Droplet::remove_overlay(QtOverlay*) {}
void Droplet::update_interval() {}
void Droplet::change_channel() 
{
  int ich = _channelBox->currentIndex();
  _channel=ich; 
}

 void Droplet::prototype(const DescEntry& i)
 {
   _prototype = static_cast<const DescImage&>(i);
 }

void Droplet::new_set()
{
  int i=_apps.size()/_nchannels;
  _configs.push_back(new Ami::DropletConfig);
  QString name = QString("Set%1").arg(i);
  _setBox->addItem(name);
  for(unsigned j=0; j<_nchannels; j++) {
    DropletConfigApp* set = new DropletConfigApp(this, name,j,*_configs[i]);
    _apps.push_back(set);
    connect(set, SIGNAL(changed()), this, SIGNAL(changed()));
  }
  _setBox->setCurrentIndex(i);
}

void Droplet::select_set(int i)
{
  if (i>=0)
    _config->load(*_configs[i]);
}

DropletConfigApp& Droplet::_app() { return *_apps[_setBox->currentIndex()*_nchannels + _channelBox->currentIndex()]; }


DropletConfigApp::DropletConfigApp(QWidget* parent,
				   const QString& s,
				   unsigned i,
				   const Ami::DropletConfig& c) :
  VAConfigApp(parent,s,i),
  _config    (c)
{
}

DropletConfigApp::~DropletConfigApp() {}

Ami::AbsOperator* DropletConfigApp::_op(const char* name)
{
  return new Ami::Droplet(name,_config);
}

DropletConfig::DropletConfig(QWidget* parent) : QWidget(parent)
{ QGridLayout* l = new QGridLayout;
  unsigned row=0;
  l->addWidget(new QLabel("Seed Threshold [ADU]"),row,0,::Qt::AlignRight);
  l->addWidget(_seed_thr = new QLineEdit("0")    ,row,1,::Qt::AlignLeft);
  new QIntValidator(_seed_thr); row++;
  connect(_seed_thr, SIGNAL(editingFinished()), this, SIGNAL(changed()));

  l->addWidget(new QLabel("Neighbor Threshold [ADU]"),row,0,::Qt::AlignRight);
  l->addWidget(_nbor_thr = new QLineEdit("0")        ,row,1,::Qt::AlignLeft);
  new QIntValidator(_nbor_thr); row++;
  connect(_nbor_thr, SIGNAL(editingFinished()), this, SIGNAL(changed()));
	
  l->addWidget(new QLabel("Esum Minimum [ADU]"),row,0,::Qt::AlignRight);
  l->addWidget(_esum_min = new QLineEdit("0")  ,row,1,::Qt::AlignLeft);
  new QIntValidator(_esum_min); row++;
  connect(_esum_min, SIGNAL(editingFinished()), this, SIGNAL(changed()));
	
  l->addWidget(new QLabel("Esum Maximum [ADU]"),row,0,::Qt::AlignRight);
  l->addWidget(_esum_max = new QLineEdit("0")  ,row,1,::Qt::AlignLeft);
  new QIntValidator(_esum_max); row++;
  connect(_esum_max, SIGNAL(editingFinished()), this, SIGNAL(changed()));
	
  l->addWidget(new QLabel("Npixel Minimum")  ,row,0,::Qt::AlignRight);
  l->addWidget(_npix_min = new QLineEdit("0"),row,1,::Qt::AlignLeft);
  new QIntValidator(_npix_min); row++;
  connect(_npix_min, SIGNAL(editingFinished()), this, SIGNAL(changed()));

  l->addWidget(new QLabel("Npixel Maximum")  ,row,0,::Qt::AlignRight);
  l->addWidget(_npix_max = new QLineEdit("0"),row,1,::Qt::AlignLeft);
  new QIntValidator(_npix_max); row++;
  connect(_npix_max, SIGNAL(editingFinished()), this, SIGNAL(changed()));

  setLayout(l);
}

DropletConfig::~DropletConfig() {}

Ami::DropletConfig DropletConfig::value() const {
  Ami::DropletConfig v;
  v.seed_threshold = _seed_thr->text().toInt();
  v.nbor_threshold = _nbor_thr->text().toInt();
  v.esum_min       = _esum_min->text().toInt();
  v.esum_max       = _esum_max->text().toInt();
  v.npix_min       = _npix_min->text().toInt();
  v.npix_max       = _npix_max->text().toInt();
  return v;
}

void DropletConfig::load(const Ami::DropletConfig& v) {
  _seed_thr->setText(QString::number(v.seed_threshold));
  _nbor_thr->setText(QString::number(v.nbor_threshold));
  _esum_min->setText(QString::number(v.esum_min));
  _esum_max->setText(QString::number(v.esum_max));
  _npix_min->setText(QString::number(v.npix_min));
  _npix_max->setText(QString::number(v.npix_max));
}

void DropletConfig::load(const char*& p) {
  XML_iterate_open(p,tag)
    if (tag.name == "_seed_thr")
      _seed_thr->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_nbor_thr")
      _nbor_thr->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_esum_min")
      _esum_min->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_esum_max")
      _esum_max->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_npix_min")
      _npix_min->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_npix_max")
      _npix_max->setText(QtPersistent::extract_s(p));
  XML_iterate_close(DropletConfig,tag);
}

void DropletConfig::save(char*& p) const {
  XML_insert(p, "QLineEdit", "_seed_thr", QtPersistent::insert(p,_seed_thr->text()) );
  XML_insert(p, "QLineEdit", "_nbor_thr", QtPersistent::insert(p,_nbor_thr->text()) );
  XML_insert(p, "QLineEdit", "_esum_min", QtPersistent::insert(p,_esum_min->text()) );
  XML_insert(p, "QLineEdit", "_esum_max", QtPersistent::insert(p,_esum_max->text()) );
  XML_insert(p, "QLineEdit", "_npix_min", QtPersistent::insert(p,_npix_min->text()) );
  XML_insert(p, "QLineEdit", "_npix_max", QtPersistent::insert(p,_npix_max->text()) );
}
