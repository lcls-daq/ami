#include "EdgeFinder.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/CursorDefinition.hh"
#include "ami/qt/EdgeCursor.hh"
#include "ami/qt/EdgePlot.hh"
#include "ami/qt/WaveformDisplay.hh"
#include "ami/qt/PlotFrame.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EdgeFinder.hh"

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
#include <QtGui/QMessageBox>

#include <sys/socket.h>

using namespace Ami::Qt;

EdgeFinder::EdgeFinder(ChannelDefinition* channels[], unsigned nchannels, WaveformDisplay& frame) :
  QWidget   (0),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _title    (new QLineEdit("Edge plot"))
{
  _baseline  = new EdgeCursor("baseline" ,*_frame.plot());
  _threshold = new EdgeCursor("threshold",*_frame.plot());

  setWindowTitle("EdgeFinder Plot");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _hist   = new DescTH1F ("Sum (1dH)");

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");
  QPushButton* loadB  = new QPushButton("Load");
  QPushButton* saveB  = new QPushButton("Save");
  
  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* file_box = new QGroupBox("File");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(loadB);
    layout1->addWidget(saveB);
    layout1->addStretch();
    file_box->setLayout(layout1);
    layout->addWidget(file_box); }
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* locations_box = new QGroupBox("Define EdgeFinder");
    locations_box->setToolTip("Define baseline and threshold values.");
    QVBoxLayout* layout2 = new QVBoxLayout;
    layout2->addWidget(_baseline);
    layout2->addWidget(_threshold);
    locations_box->setLayout(layout2);
    layout->addWidget(locations_box); }
  { QGroupBox* plot_box = new QGroupBox("Plot");
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(new QLabel("Title"));
      layout2->addWidget(_title);
      layout1->addLayout(layout2); }
    layout1->addWidget(_hist );
    plot_box->setLayout(layout1); 
    layout->addWidget(plot_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);
    
  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(loadB     , SIGNAL(clicked()),      this, SLOT(load()));
  connect(saveB     , SIGNAL(clicked()),      this, SLOT(save()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
}
  
EdgeFinder::~EdgeFinder()
{
}

void EdgeFinder::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels)
{
  for(std::list<EdgePlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->configure(p,input,output,channels,signatures,nchannels,_frame.xinfo());
}

void EdgeFinder::setup_payload(Cds& cds)
{
  for(std::list<EdgePlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
}

void EdgeFinder::update()
{
  for(std::list<EdgePlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
}

void EdgeFinder::set_channel(int c) 
{ 
  _channel=c; 
}

void EdgeFinder::load()
{
}

void EdgeFinder::save()
{
}

void EdgeFinder::plot()
{
  Ami::DescTH1F desc(qPrintable(_title->text()),
		     "edge location","pulses",
		     _hist->bins(),_hist->lo(),_hist->hi()); 
  
  EdgePlot* plot = new EdgePlot(_title->text(),
				new Ami::EdgeFinder(_channel,
						    0.5,
						    _threshold->value(),
						    _baseline ->value(),
						    desc));
  _plots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void EdgeFinder::remove_plot(QObject* obj)
{
  EdgePlot* plot = static_cast<EdgePlot*>(obj);
  _plots.remove(plot);

  disconnect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

