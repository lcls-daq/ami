#include "PeakFinder.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/PeakPlot.hh"
#include "ami/qt/ZoomPlot.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageScale.hh"
#include "ami/qt/SMPRegistry.hh"
#include "ami/qt/SMPWarning.hh"
#include "ami/qt/ControlLog.hh"

#include "ami/data/DescImage.hh"
#include "ami/data/PeakFinder.hh"

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

static inline int avgRound(int n, int d)
{
  return (n+d-1)/d;
}

using namespace Ami::Qt;

PeakFinder::PeakFinder(QWidget* parent,
		       ChannelDefinition* channels[], unsigned nchannels) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _list_sem (Semaphore::FULL)
{
  _threshold = new ImageScale("threshold");
  
  _accumulate = new QCheckBox("accumulate events");
  _accumulate->setChecked(true);

  _interval  = new QLineEdit;
  _intervalq = new QLabel;
  new QIntValidator(_interval);
  
  _smp_warning = new SMPWarning;

  _center_only = new QCheckBox("local max only");
  _center_only->setChecked(true);

  setWindowTitle("PeakFinder Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _proc_grp = new QButtonGroup;
  QRadioButton* countB = new QRadioButton("count hits");
  QRadioButton* sumB   = new QRadioButton("sum values");
  _proc_grp->addButton(countB, Ami::PeakFinder::Count);
  _proc_grp->addButton(sumB  , Ami::PeakFinder::Sum);

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");
  
  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* locations_box = new QGroupBox("Counting Threshold");
    locations_box->setToolTip("Define threshold value.");
    QVBoxLayout* layout2 = new QVBoxLayout;
    layout2->addWidget(_threshold);
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(countB);
    layout1->addStretch();
    layout1->addWidget(sumB); 
    layout->addLayout(layout1); }
  { layout->addWidget(_center_only); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_accumulate);
    layout1->addWidget(_interval);
    layout1->addWidget(_intervalq);
    layout1->addWidget(_smp_warning);
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(_interval , SIGNAL(editingFinished()), this, SLOT(update_interval()));
  connect(&SMPRegistry::instance(), SIGNAL(changed()), this, SLOT(update_interval()));
  connect(_accumulate, SIGNAL(clicked()),     this, SLOT(update_interval()));
  connect(channelBox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_channel()));
  for(unsigned i=0; i<_nchannels; i++)
    connect(_channels[i], SIGNAL(agg_changed()), this, SLOT(change_channel()));

  update_interval();
  _proc_grp->button(Ami::PeakFinder::Count)->setChecked(true);

  _channelBox = channelBox;
  _plotB = plotB;
}
  
PeakFinder::~PeakFinder()
{
}

void PeakFinder::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert(p, "int", "_channel", QtPersistent::insert(p,_channel) );
  XML_insert(p, "QLineEdit", "_threshold_0", QtPersistent::insert(p,_threshold->value(0)) );
  XML_insert(p, "QLineEdit", "_threshold_1", QtPersistent::insert(p,_threshold->value(1)) );
  XML_insert(p, "QButtonGroup", "_proc_grp", QtPersistent::insert(p,_proc_grp->checkedId()) );
  XML_insert(p, "QCheckBox", "_center_only", QtPersistent::insert(p,_center_only->isChecked()) );
  XML_insert(p, "QCheckBox", "_accumulate", QtPersistent::insert(p,_accumulate->isChecked()) );
  XML_insert( p, "QLineEdit"   , "_interval", QtPersistent::insert(p,_interval->text()) );

  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert(p, "PeakPlot", "_plots", (*it)->save(p) );
  }
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++) {
    XML_insert(p, "ZoomPlot", "_zplots", (*it)->save(p) );
  }
}

void PeakFinder::load(const char*& p)
{
  _list_sem.take();

  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(description_changed()), this, SIGNAL(changed()));
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  }
  _plots.clear();

  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _zplots.clear();

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_threshold_0")
      _threshold->value(0,QtPersistent::extract_d(p));
    else if (tag.name == "_threshold_1")
      _threshold->value(1,QtPersistent::extract_d(p));
    else if (tag.name == "_accumulate")
      _accumulate->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_interval")
      _interval->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_center_only")
      _center_only->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_proc_grp")
      _proc_grp->button(QtPersistent::extract_i(p))->setChecked(true);
    else if (tag.name == "_plots") {
      PeakPlot* plot = new PeakPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_zplots") {
      ZoomPlot* plot = new ZoomPlot(this, p);
      _zplots.push_back(plot);
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
  XML_iterate_close(PeakFinder,tag);

  _list_sem.give();

  update_interval();
}

void PeakFinder::save_plots(const QString& p) const
{
}

void PeakFinder::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  _list_sem.take();

  unsigned mask=0;
  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,input,output,channels,signatures,nchannels);
    else
      mask |= 1<<(*it)->channel();
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels);

  _list_sem.give();

  if (mask) {
    for(unsigned ich=0; ich<_nchannels; ich++) {
      ControlLog& log = ControlLog::instance();
      if (mask & (1<<ich)) {
	QString s = QString("Hit Finder plots/posts for %1 disabled [SMP]")
	  .arg(channels[ich]->name());
	log.appendText(s);
      }
    }
  }
}

void PeakFinder::setup_payload(Cds& cds)
{
  _list_sem.take();
  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
   (*it)->setup_payload(cds);
  _list_sem.give();
}

void PeakFinder::update()
{
  _list_sem.take();
  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->update();
  _list_sem.give();
}

void PeakFinder::prototype(const DescEntry& e)
{
  _threshold->prototype(e);
}

void PeakFinder::set_channel(int c) 
{ 
  _channel=c; 
}

void PeakFinder::plot()
{
  unsigned nproc = SMPRegistry::instance().nservers();
  Ami::PeakFinder* op = new Ami::PeakFinder(_threshold->value(0),
					    _threshold->value(1),
					    Ami::PeakFinder::Mode(_proc_grp->checkedId()),
					    _center_only->isChecked(),
					    _accumulate->isChecked() ? avgRound(_interval->text().toInt(),nproc) : -1);

#if 0
  if (nproc>1 && _accumulate->isChecked()) {
    ZoomPlot* plot = new ZoomPlot(this,
				  QString("%1 HitFinder : %2,%3").arg(_channels[_channel]->name())
				  .arg(_threshold->value(0))
				  .arg(_threshold->value(1)),
				  _channel,
				  op);
    _list_sem.take();
    _zplots.push_back(plot);
    _list_sem.give();
    connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  }
  else {
    bool displayOnly=false;
#else
  {
    bool displayOnly=nproc>1 && _accumulate->isChecked();
#endif
    PeakPlot* plot = new PeakPlot(this,
				  QString("%1 HitFinder : %2,%3").arg(_channels[_channel]->name())
				  .arg(_threshold->value(0))
				  .arg(_threshold->value(1)),
				  _channel,
				  op,
				  displayOnly);
    _list_sem.take();
    _plots.push_back(plot);
    _list_sem.give();
    
    connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
    connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  }

  emit changed();
}

void PeakFinder::remove_plot(QObject* obj)
{
  _list_sem.take();
  { PeakPlot* plot = static_cast<PeakPlot*>(obj);
    _plots.remove(plot); }

  { ZoomPlot* plot = static_cast<ZoomPlot*>(obj);
    _zplots.remove(plot); }

  disconnect(obj, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  delete obj;
  _list_sem.give();

  emit changed();
}

void PeakFinder::update_interval()
{
  unsigned nproc = SMPRegistry::instance().nservers();
  int n = _interval->text().toInt();
  int m = nproc*avgRound(n,nproc);
  if (n>1 && m!=n)
    _intervalq->setText(QString("(%1)").arg(QString::number(m)));
  else
    _intervalq->clear();

  bool checked = _accumulate->checkState()==::Qt::Checked;
  _interval   ->setEnabled(checked);
  _smp_warning->setEnabled(checked);
}

void PeakFinder::change_channel()
{
  int ich = _channelBox->currentIndex();
  bool enable = !_channels[ich]->smp_prohibit();
  _plotB->setEnabled(enable);
}
