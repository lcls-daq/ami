#include "FFT.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/ProjectionPlot.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/FFT.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>

#include <sys/socket.h>
#include <stdio.h>

using namespace Ami::Qt;

FFT::FFT(QWidget* parent, 
	 ChannelDefinition* channels[], 
	 unsigned nchannels) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0)
{
  setWindowTitle("FFT Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  QStringList parms;
  for(unsigned i=0; i<Ami::FFT::NumberOf; i++)
    parms << Ami::FFT::parameter(Ami::FFT::Parameter(i));
  _parameter = new QComboBox;
  _parameter->addItems(parms);

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
  layout->addWidget(_parameter);
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
}
  
FFT::~FFT()
{
  for(std::list<ProjectionPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    delete *it;
  }
  _plots.clear();
}

void FFT::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  for(std::list<ProjectionPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert(p, "ProjectionPlot", "_plots", (*it)->save(p) );
  }
}

void FFT::load(const char*& p)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
    delete *it;
  }
  _plots.clear();

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_plots") {
      ProjectionPlot* plot = new ProjectionPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
  XML_iterate_close(FFT,tag);

  setVisible(false);

  emit changed();
}

void FFT::save_plots(const QString& p) const
{
  for(std::list<ProjectionPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QString s = QString("%1_%2").arg(p).arg((*it)->_name);
    (*it)->save_plots(s);
  }
}

void FFT::configure(char*& p, unsigned input, unsigned& output,
			 ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			 ConfigureRequest::Source source)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,input,output,channels,signatures,nchannels);
}

void FFT::setup_payload(Cds& cds)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
}

void FFT::update()
{
  for(std::list<ProjectionPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
}

void FFT::initialize(const DescEntry& e)
{
}

void FFT::set_channel(int c) 
{ 
  _channel=c; 
}

void FFT::plot()
{
  QString s = QString("%1 FFT[%2]")
    .arg(_channels[_channel]->name())
    .arg(_parameter->currentText());
  ProjectionPlot* plot = new ProjectionPlot(this,
					    qPrintable(s),
					    _channel,
					    new Ami::FFT(Ami::FFT::Parameter(_parameter->currentIndex())));

  _plots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  connect(plot, SIGNAL(changed()), this, SIGNAL(changed()));

  emit changed();
}

void FFT::remove_plot(QObject* obj)
{
  ProjectionPlot* plot = static_cast<ProjectionPlot*>(obj);
  _plots.remove(plot);

  disconnect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

