#include "BlobFinder.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/RectangleCursors.hh"
#include "ami/qt/PeakPlot.hh"
#include "ami/qt/ZoomPlot.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageScale.hh"
#include "ami/qt/SMPRegistry.hh"
#include "ami/qt/SMPWarning.hh"
#include "ami/qt/ControlLog.hh"

#include "ami/data/DescImage.hh"
#include "ami/data/BlobFinder.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QIntValidator>
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>
#include <QtGui/QCheckBox>

#include <sys/socket.h>

using namespace Ami::Qt;

BlobFinder::BlobFinder(QtPWidget* parent,
		       ChannelDefinition* channels[], unsigned nchannels, ImageFrame& frame) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _list_sem (Semaphore::FULL)
{
  _rectangle = new RectangleCursors(_frame, parent);
  _threshold = new ImageScale("threshold");
  _cluster_size = new QLineEdit();
  new QIntValidator(1,1000000,_cluster_size);
  _accumulate = new QCheckBox("sum events");
  _accumulate->setChecked(true);
  _smp_warning = new SMPWarning;

  setWindowTitle("BlobFinder Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

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
  { QGroupBox* locations_box = new QGroupBox("Define Boundaries");
    locations_box->setToolTip("Define search boundaries.");
    QVBoxLayout* layout2 = new QVBoxLayout;
    layout2->addWidget(_rectangle);
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QGroupBox* locations_box = new QGroupBox("Blob Properties");
    locations_box->setToolTip("Define required blob properties.");
    QVBoxLayout* layout2 = new QVBoxLayout;
    { QHBoxLayout* hl = new QHBoxLayout;
      hl->addWidget(new QLabel("Min. Size"));
      hl->addWidget(_cluster_size);
      layout2->addLayout(hl); }
    layout2->addWidget(_threshold);
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_accumulate);
    layout1->addWidget(_smp_warning);
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  layout->addStretch();
  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(_rectangle, SIGNAL(changed()),      this, SLOT(update_range()));
  connect(_rectangle, SIGNAL(done()),         this, SLOT(front()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(closeB    , SIGNAL(clicked()),      this, SIGNAL(closed()));
  connect(channelBox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_channel()));
  for(unsigned i=0; i<_nchannels; i++)
    connect(_channels[i], SIGNAL(agg_changed()), this, SLOT(change_channel()));

  _channelBox = channelBox;
  _plotB = plotB;
}
  
BlobFinder::~BlobFinder()
{
}

void BlobFinder::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert(p, "int", "_channel", QtPersistent::insert(p,_channel) );
  XML_insert(p, "RectangleCursors", "_rectangle", _rectangle->save(p) );
  XML_insert(p, "QLineEdit", "_cluster_size", QtPersistent::insert(p, _cluster_size->text()) );
  XML_insert(p, "QLineEdit", "_threshold_0", QtPersistent::insert(p,_threshold->value(0)) );
  XML_insert(p, "QLineEdit", "_threshold_1", QtPersistent::insert(p,_threshold->value(1)) );
  XML_insert(p, "QCheckBox", "_accumulate", QtPersistent::insert(p,_accumulate->isChecked()) );

  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert(p, "PeakPlot", "_plots", (*it)->save(p) );
  }
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++) {
    XML_insert(p, "ZoomPlot", "_zplots", (*it)->save(p) );
  }
}

void BlobFinder::load(const char*& p)
{
  _list_sem.take();

  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(description_changed()), this, SIGNAL(changed()));
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  }
  _plots.clear();

  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++) {
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  }
  _zplots.clear();

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_rectangle")
      _rectangle->load(p);
    else if (tag.name == "_cluster_size")
      _cluster_size->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_threshold_0")
      _threshold->value(0,QtPersistent::extract_d(p));
    else if (tag.name == "_threshold_1")
      _threshold->value(1,QtPersistent::extract_d(p));
    else if (tag.name == "_accumulate")
      _accumulate->setChecked(QtPersistent::extract_b(p));
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
  XML_iterate_close(BlobFinder,tag);

  _list_sem.give();
}

void BlobFinder::snapshot(const QString& p) const
{
  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg("hitfinder"));
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg("zoom"));
}

void BlobFinder::save_plots(const QString& p) const
{
}

void BlobFinder::update_range()
{
  _frame.replot();
}

void BlobFinder::setVisible(bool v)
{
  if (v)    _frame.add_marker(*_rectangle);
  else      _frame.remove_marker(*_rectangle);
  QWidget::setVisible(v);
}

void BlobFinder::configure(char*& p, unsigned input, unsigned& output,
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
	QString s = QString("Blob Finder plots/posts for %1 disabled [SMP]")
	  .arg(channels[ich]->name());
	log.appendText(s);
      }
    }
  }
}

void BlobFinder::setup_payload(Cds& cds)
{
  _list_sem.take();

  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->setup_payload(cds);

  _list_sem.give();
}

void BlobFinder::update()
{
  _list_sem.take();

  for(std::list<PeakPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->update();

  _list_sem.give();
}

void BlobFinder::prototype(const DescEntry& e)
{
  _threshold->prototype(e);
}

void BlobFinder::set_channel(int c) 
{ 
  _channel=c; 
}

void BlobFinder::plot()
{
  Ami::BlobFinder* op = new Ami::BlobFinder(_rectangle->iylo(),
					    _rectangle->iyhi(),
					    _rectangle->ixlo(),
					    _rectangle->ixhi(),
					    unsigned(_threshold->value(0)),
					    _cluster_size->text().toInt(),
					    _accumulate->isChecked());

  unsigned nproc = SMPRegistry::instance().nservers();
  if (nproc>1 && _accumulate->isChecked()) {
    ZoomPlot* plot = new ZoomPlot(this,
				  QString("%1 BlobFinder : %2,%3").arg(_channels[_channel]->name())
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
    PeakPlot* plot = new PeakPlot(this,
				  QString("%1 BlobFinder : %2,%3").arg(_channels[_channel]->name())
				  .arg(_threshold->value(0))
				  .arg(_threshold->value(1)),
				  _channel,
				  op);
    _list_sem.take();
    _plots.push_back(plot);
    _list_sem.give();

    connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
    connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  }

  emit changed();
}

void BlobFinder::remove_plot(QObject* obj)
{
  _list_sem.take();
  { PeakPlot* plot = static_cast<PeakPlot*>(obj);
    _plots.remove(plot); }

  { ZoomPlot* plot = static_cast<ZoomPlot*>(obj);
    _zplots.remove(plot); }

  disconnect(obj, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  delete obj;
  _list_sem.give();
}

void BlobFinder::change_channel()
{
  int ich = _channelBox->currentIndex();
  bool enable = !_channels[ich]->smp_prohibit();
  _plotB->setEnabled(enable);
}
