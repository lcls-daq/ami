#include "Client.hh"

#include "ami/qt/Control.hh"
#include "ami/qt/EdgeFinder.hh"
#include "ami/qt/CursorsX.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/Display.hh"
#include "ami/qt/Math.hh"
#include "ami/qt/Status.hh"
#include "ami/qt/QtBase.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/FeatureRegistry.hh"

#include "ami/client/VClientManager.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/service/Socket.hh"

#include "pdsdata/xtc/ClockTime.hh"

#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>

#include <sys/types.h>
#include <sys/socket.h>

using namespace Ami;

typedef Pds::DetInfo DI;

static const int BufferSize = 0x8000;

Ami::Qt::Client::Client(const Pds::DetInfo& src,
			unsigned            channel) :
  QWidget          (0),
  _src             (src),
  _channel         (channel),
  _input_entry     (0),
  _output_signature(0), 
  _request         (new char[BufferSize]),
  _description     (new char[BufferSize]),
  _cds             ("Client"),
  _niovload        (5),
  _iovload         (new iovec[_niovload])
{
  if (src.detector()==Pds::DetInfo::AmoETof)
    setWindowTitle(QString("AmoETof-%1").arg(channel));
  else
    setWindowTitle(QString("%1").arg(Pds::DetInfo::name(src.detector())));

  setAttribute(::Qt::WA_DeleteOnClose, false);

  _control = new Control(*this);
  _frame   = new Display;
  _status  = new Status;


  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* layout3 = new QVBoxLayout;
    { QGroupBox* chanBox = new QGroupBox("Channels");
      QVBoxLayout* layout1 = new QVBoxLayout;
      QPushButton* chanB[NCHANNELS];
      QColor color[] = { QColor(0,0,255), QColor(255,0,0), QColor(0,255,0), QColor(255,0,255) };
      for(int i=0; i<NCHANNELS; i++) {
	QString title = QString("Ch%1").arg(char('A'+i));
	_channels[i] = new ChannelDefinition(title, *_frame, color[i], i==0);
	chanB[i] = new QPushButton(title); chanB[i]->setCheckable(true);
	chanB[i]->setPalette(QPalette(color[i]));
	layout1->addWidget(chanB[i]);
	connect(chanB[i], SIGNAL(clicked(bool)), _channels[i], SLOT(setVisible(bool)));
	connect(_channels[i], SIGNAL(changed()), this, SLOT(update_configuration()));
      }
      chanBox->setLayout(layout1);
      layout3->addWidget(chanBox); }
    { QPushButton* edgesB = new QPushButton("Edges");
      edgesB->setCheckable(true);
      layout3->addWidget(edgesB);
      _edges = new EdgeFinder(_channels,NCHANNELS,*_frame);
      connect(edgesB, SIGNAL(clicked(bool)), _edges, SLOT(setVisible(bool))); }
    { QPushButton* cursorsB = new QPushButton("Cursors");
      cursorsB->setCheckable(true);
      layout3->addWidget(cursorsB);
      _cursors = new CursorsX(_channels,NCHANNELS,*_frame);
      connect(cursorsB, SIGNAL(clicked(bool)), _cursors, SLOT(setVisible(bool))); }
    layout3->addStretch();
    layout->addLayout(layout3); }
  { QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(_control);
      layout2->addStretch();
      layout2->addWidget(_status);
      layout1->addLayout(layout2); }
    layout1->addWidget(_frame);
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(this, SIGNAL(description_changed(int)), this, SLOT(_read_description(int)));
  connect(_cursors, SIGNAL(changed()), this, SLOT(update_configuration()));
  connect(_edges  , SIGNAL(changed()), this, SLOT(update_configuration()));
}

Ami::Qt::Client::~Client() 
{
  delete[] _iovload;
  delete[] _description;
  delete[] _request; 
}

void Ami::Qt::Client::connected()
{
  _status->set_state(Status::Connected);
  //  _manager->configure();
  _manager->discover();
}

void Ami::Qt::Client::discovered(const DiscoveryRx& rx)
{
  _status->set_state(Status::Discovered);
  printf("Discovered\n");

  //  iterate through discovery and print
  FeatureRegistry::instance().clear ();
  FeatureRegistry::instance().insert(rx.feature_name(0),rx.features());

  char channel_name [128]; 
  strcpy(channel_name ,ChannelID::name(_src,_channel));
  _input_entry = 0;

  for(  const DescEntry* e = rx.entries(); e < rx.end(); 
      e = reinterpret_cast<const DescEntry*>
	(reinterpret_cast<const char*>(e) + e->size())) {
    if (strcmp(e->name(),channel_name)==0) {
      _input_entry = e;
    }
  }

  if (_input_entry==0)
    printf("%s not found\n", channel_name);

  _manager->configure();
}

int  Ami::Qt::Client::configure       (iovec* iov) 
{
  printf("Configure\n");
  if (_input_entry==0) {
    printf("input_entry not found\n");
    return 0;
  }
  else {

    int signatures[NCHANNELS];
    for(int i=0; i<NCHANNELS; i++)
      signatures[i] = -1;

    char* p = _request;

    //
    //  Configure channels which depend upon others
    //
    bool lAdded;
    do {
      lAdded=false;
      for(unsigned i=0; i<NCHANNELS; i++) {
	if (signatures[i]<0) {
	  int sig = _channels[i]->configure(p,_input_entry->signature(),_output_signature,
					    _channels,signatures,NCHANNELS);
	  if (sig >= 0) {
	    signatures[i] = sig;
	    lAdded = true;
	    //	    printf("Added signature %d for channel %d\n",sig,i);
	  }
	}
      }
    } while(lAdded);

    _edges  ->configure(p,_input_entry->signature(),_output_signature,
			_channels,signatures,NCHANNELS);

    _cursors->configure(p,_input_entry->signature(),_output_signature,
			_channels,signatures,NCHANNELS);

    if (p > _request+BufferSize) {
      printf("Client request overflow: size = 0x%x\n", p-_request);
      return 0;
    }
    else {
      iov[0].iov_base = _request;
      iov[0].iov_len  = p - _request;
      return 1;
    }
  }
}

//
//  This state is unnecessary
//
int  Ami::Qt::Client::configured      () 
{
  printf("Configured\n");
  //  _status->set_state(Status::Configured);
  return 0; 
}

//
//  read_description adds/removes plots
//
void Ami::Qt::Client::read_description(Socket& socket)
{
  printf("Described so\n");
  int size = socket.read(_description,BufferSize);

  if (size<0) {
    printf("Read error in Ami::Qt::Client::read_description.\n");
    return;
  }

  if (size==BufferSize) {
    printf("Buffer overflow in Ami::Qt::Client::read_description.  Dying...\n");
    abort();
  }

  //  printf("emit description\n");
  emit description_changed(size);
}

void Ami::Qt::Client::_read_description(int size)
{
  printf("Described si\n");
  //  printf("description\n"); 

  _frame->reset();
  _cds.reset();

  const char* payload = _description;
  const char* const end = payload + size;
  //  dump(payload, size);

  { const Desc* e = reinterpret_cast<const Desc*>(payload);
    //    printf("Cds %s\n",e->name());
    payload += sizeof(Desc); }

  while( payload < end ) {
    const DescEntry* desc = reinterpret_cast<const DescEntry*>(payload);
    if (desc->size()==0) {
      printf("read_description size==0\n");
      break;
    }
    Entry* entry = EntryFactory::entry(*desc);
    _cds.add(entry, desc->signature());
    payload += desc->size();
//     printf("Received desc %s signature %d\n",desc->name(),desc->signature());
  }

  if (_cds.totalentries()>_niovload) {
    delete[] _iovload;
    _iovload = new iovec[_niovload=_cds.totalentries()];
  }
  _cds.payload(_iovload);

  for(unsigned i=0; i<NCHANNELS; i++)
    _channels[i]->setup_payload(_cds);

  _edges  ->setup_payload(_cds);

  _cursors->setup_payload(_cds);

  _status->set_state(Status::Described);
}

//
//  read_payload changes plot contents
//
void Ami::Qt::Client::read_payload     (Socket& socket)
{
  //  printf("payload\n"); 
  if (_status->state() == Status::Requested) {
    socket.readv(_iovload,_cds.totalentries());
  }
  else if (!_one_shot) {
    //
    //  Add together each server's contribution
    //
    printf("Ami::Qt::Client::read_payload: multiple server processing not implemented\n");
  }
  _status->set_state(Status::Received);
}

void Ami::Qt::Client::process         () 
{
  //  printf("process\n"); 

  //
  //  Perform client-side processing
  //
  _frame  ->update();
  _edges  ->update();
  _cursors->update();
  
  _status->set_state(Status::Processed);
}

void Ami::Qt::Client::managed(VClientManager& mgr)
{
  _manager = &mgr;
  show();
}

void Ami::Qt::Client::request_payload()
{
  if (_status->state() >= Status::Described) {
    _manager->request_payload();
    _status->set_state(Status::Requested);
  }
}

void Ami::Qt::Client::one_shot(bool l) 
{
  _one_shot=l; 
}

void Ami::Qt::Client::update_configuration()
{
  _manager->configure();
}
