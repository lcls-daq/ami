#include "WaveformClient.hh"

#include "ami/qt/WaveformDisplay.hh"
#include "ami/qt/EdgeFinder.hh"
#include "ami/qt/CursorsX.hh"

#include <QtGui/QPushButton>

using namespace Ami::Qt;

WaveformClient::WaveformClient(QWidget* parent,const Pds::DetInfo& info, unsigned ch) :
  Client  (parent,info,ch, new WaveformDisplay)
{
  WaveformDisplay& wd = static_cast<WaveformDisplay&>(display());

  { QPushButton* edgesB = new QPushButton("Edges");
    edgesB->setCheckable(true);
    addWidget(edgesB);
    _edges = new EdgeFinder(_channels,NCHANNELS,wd);
    connect(edgesB, SIGNAL(clicked(bool)), _edges, SLOT(setVisible(bool))); }
  { QPushButton* cursorsB = new QPushButton("Cursors");
    cursorsB->setCheckable(true);
    addWidget(cursorsB);
    _cursors = new CursorsX(_channels,NCHANNELS,wd);
    connect(cursorsB, SIGNAL(clicked(bool)), _cursors, SLOT(setVisible(bool))); }

  connect(_cursors, SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_edges  , SIGNAL(changed()), this, SLOT(update_configuration()));
}

WaveformClient::~WaveformClient() {}

void WaveformClient::_configure(char*& p, 
				unsigned input, 
				unsigned& output,
				ChannelDefinition* ch[], 
				int* signatures, 
				unsigned nchannels)
{
  _edges  ->configure(p, input, output,
		      ch, signatures, nchannels);
  _cursors->configure(p, input, output,
		      ch, signatures, nchannels,
		      ConfigureRequest::Analysis);
}

void WaveformClient::_setup_payload(Cds& cds)
{
  _edges  ->setup_payload(cds);
  _cursors->setup_payload(cds);
}

void WaveformClient::_update()
{
  _edges  ->update();
  _cursors->update();
}
