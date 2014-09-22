#include "WaveformClient.hh"

#include "ami/qt/WaveformDisplay.hh"
#include "ami/qt/EdgeFinder.hh"
#include "ami/qt/CursorsX.hh"
#include "ami/qt/CurveFit.hh"
#include "ami/qt/FFT.hh"

#include <QtGui/QPushButton>

#include <stdio.h>

using namespace Ami::Qt;

WaveformClient::WaveformClient(QWidget* parent,const Pds::DetInfo& info, unsigned ch, const QString& name) :
  Client  (parent,info,ch,name, new WaveformDisplay),
  _initialized(false)
{
  WaveformDisplay& wd = static_cast<WaveformDisplay&>(display());
  connect(&wd, SIGNAL(set_chrome_visible(bool)), this, SLOT(set_chrome_visible(bool)));

  { QPushButton* edgesB = new QPushButton("Edges");
    addWidget(edgesB);
    _edges = new EdgeFinder(this,_channels,NCHANNELS,wd,this);
    _stack->add(edgesB,_edges);
    connect(_edges  , SIGNAL(changed()), this, SIGNAL(changed())); }
  { QPushButton* cursorsB = new QPushButton("Cursors");
    addWidget(cursorsB);
    _cursors = new CursorsX(this,_channels,NCHANNELS,wd, this);
    _stack->add(cursorsB,_cursors);
    connect(_cursors, SIGNAL(changed()), this, SIGNAL(changed())); }
  { QPushButton* fitB = new QPushButton("Waveform Fit");
    addWidget(fitB);
    _fits = new CurveFit(this,_channels,NCHANNELS,wd);
    _stack->add(fitB,_fits);
    connect(_fits   , SIGNAL(changed()), this, SIGNAL(changed())); }
  { QPushButton* fftB = new QPushButton("Waveform FFT");
    addWidget(fftB);
    _fft = new FFT(this,_channels,NCHANNELS);
    _stack->add(fftB,_fft);
    connect(_fft    , SIGNAL(changed()), this, SIGNAL(changed())); }

  set_chrome_visible(true);
}

WaveformClient::~WaveformClient() {}

void WaveformClient::save(char*& p) const
{
  XML_insert(p, "Client", "self", Client::save(p) );
  
  XML_insert(p, "EdgeFinder", "_edges", _edges  ->save(p) );
  XML_insert(p, "CursorsX", "_cursors", _cursors->save(p) );
  XML_insert(p, "CurveFit", "_fits",    _fits->save(p) );
  XML_insert(p, "FFT"     , "_fft" ,    _fft ->save(p) );
}

void WaveformClient::load(const char*& p)
{
  _initialized = true;

  disconnect(_cursors, SIGNAL(changed()), this, SIGNAL(changed()));
  disconnect(_edges  , SIGNAL(changed()), this, SIGNAL(changed()));
  disconnect(_fits   , SIGNAL(changed()), this, SIGNAL(changed()));
  disconnect(_fft    , SIGNAL(changed()), this, SIGNAL(changed()));
  
  XML_iterate_open(p,tag)

    if (tag.element == "Client")
      Client::load(p);
    else if (tag.name == "_edges")
      _edges  ->load(p);
    else if (tag.name == "_cursors")
      _cursors->load(p);
    else if (tag.name == "_fits")
      _fits->load(p);
    else if (tag.name == "_fft")
      _fft ->load(p);

  XML_iterate_close(WaveformClient,tag);

  connect(_cursors, SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_edges  , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_fits   , SIGNAL(changed()), this, SIGNAL(changed()));
  connect(_fft    , SIGNAL(changed()), this, SIGNAL(changed()));

  update_configuration();
}

void WaveformClient::save_plots(const QString& p) const
{
  const WaveformDisplay& wd = static_cast<const WaveformDisplay&>(display());
  wd.save_plots(p);
  _edges  ->save_plots(p+"_edge");
  _cursors->save_plots(p+"_cursor");
  _fits   ->save_plots(p+"_fit");
  _fft    ->save_plots(p+"_fft");
}

void WaveformClient::_prototype(const DescEntry& e)
{
  if (!_initialized) {
    _initialized = true;
    
    _cursors->initialize(e);
    _fits   ->initialize(e);
  }
}

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
  _fits   ->configure(p, input, output,
		      ch, signatures, nchannels);
  _fft    ->configure(p, input, output,
		      ch, signatures, nchannels,
		      ConfigureRequest::Analysis);
}

void WaveformClient::_setup_payload(Cds& cds)
{
  _edges  ->setup_payload(cds);
  _cursors->setup_payload(cds);
  _fits   ->setup_payload(cds);
  _fft    ->setup_payload(cds);
}

void WaveformClient::_update()
{
  _edges  ->update();
  _cursors->update();
  _fits   ->update();
  _fft    ->update();
}
