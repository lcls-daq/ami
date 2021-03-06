#include "Client.hh"

#include "ami/qt/Control.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/Display.hh"
#include "ami/qt/Status.hh"
#include "ami/qt/QtBase.hh"
#include "ami/qt/ChannelDefinition.hh"
//#include "ami/qt/AggChannels.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/QtUtils.hh"
#include "ami/qt/Defaults.hh"
#include "ami/qt/ControlLog.hh"

#include "ami/client/ClientManager.hh"
#include "ami/data/Channel.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/service/Socket.hh"
#include "ami/service/Semaphore.hh"
#include "ami/service/DataLock.hh"

#include "pdsdata/xtc/ClockTime.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QScrollArea>

#include <sys/types.h>
#include <sys/socket.h>

//#define DBUG

using namespace Ami;

typedef Pds::DetInfo DI;

static const int BufferSize = 0x40000; // 256 kB

static bool _use_scroll_area=false;

Ami::Qt::Client::Client(QWidget*            parent,
			const Pds::DetInfo& src,
			unsigned            channel,
			const QString&      name,
			Display*            frame,
			double              request_rate) :
  Ami::Qt::AbsClient(parent,src,channel),
  _frame           (frame),
  _input_entry     (0),
  _input           (0),
  _title           (name),
  _output_signature(0), 
  _request         (BufferSize),
  _description     (BufferSize),
  _cds             ("Client"),
  _manager         (0),
  _niovload        (5),
  _niovread        (5),
  _iovload         (new iovec[_niovload]),
  _stack           (new QtPStack),
  _layout          (new QVBoxLayout),
  _chrome_changed  (false),
  _sem             (new Semaphore(Semaphore::EMPTY)),
  _throttled       (false),
  _denials         (0),
  _attempts        (0),
  _reset           (false)
{
  if (!request_rate) request_rate = Defaults::instance()->other_update_rate();

  setWindowTitle(QString("%1[*]").arg(name));

  setAttribute(::Qt::WA_DeleteOnClose, false);

  _control = new Control(*this,request_rate,false);
  _status  = new Status;

  QButtonGroup* showPlotBoxes = new QButtonGroup;
  showPlotBoxes->setExclusive( !frame->canOverlay() );



  QStringList refnames;
  { std::vector<unsigned> ch(Channel(channel).channels());
    for(unsigned i=0; i<ch.size(); i++)
      refnames << QString("Chan %1").arg(ch[i]+1);
  }

  _channels.resize(refnames.size()<4 ? 4:refnames.size());
  QStringList names;

  for(unsigned i=0; i<NCHANNELS; i++)
    names << QString("%1_Ch%2").arg(_title).arg(char('A'+i));
  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* layout3 = new QVBoxLayout;
    { QGroupBox* ctrlBox = new QGroupBox("Control");
      QVBoxLayout* layout1 = new QVBoxLayout;
      layout1->addWidget(_control);
      layout1->addWidget(_status); 
      ctrlBox->setLayout(layout1);
      layout3->addWidget(ctrlBox); }
    { QGroupBox* chanBox = new QGroupBox("Channels");
      QVBoxLayout* layout1 = new QVBoxLayout;
      QPushButton* chanB[NCHANNELS];
	unsigned i;
	unsigned j;
        QColor color;
      for(i=0, j=0; i<NCHANNELS; i++) {

        j = i*255/NCHANNELS;
  	if (j<43) color = QColor(j*6,0,0);
  	else if (j<129) color = QColor(255-(j-43)*3,(j-43)*3,0);
  	else if (j<215) color = QColor(0,255-(j-129)*3,(j-129)*3);
  	else if (j<255) color = QColor((j-215)*3,0,255-(j-215)*3);

	bool init = i==0 || int(i)<refnames.size();
	QString title = names[i];
	_channels[i] = new ChannelDefinition(this,title, _channels.data(), i, NCHANNELS,
					     *_frame, color, init, refnames);
	chanB[i] = new QPushButton(QString("Ch%1").arg(char('A'+i))); chanB[i]->setCheckable(false);
	chanB[i]->setPalette(QPalette(color));
	{ QHBoxLayout* layout4 = new QHBoxLayout;
	  QCheckBox* box = new QCheckBox("");
	  showPlotBoxes->addButton(box);
	  connect(box, SIGNAL(toggled(bool)), _channels[i], SLOT(show_plot(bool)));
	  connect(_channels[i], SIGNAL(show_plot_changed(bool)), box, SLOT(setChecked(bool)));
	  box->setChecked( init );
	  layout4->addWidget(box);
	  layout4->addWidget(chanB[i]);
	  layout1->addLayout(layout4);
          _stack->add(chanB[i],_channels[i]);
	  connect(_channels[i], SIGNAL(changed()), this, SIGNAL(changed()));
	  connect(_channels[i], SIGNAL(newplot(bool)), box , SLOT(setChecked(bool))); 
	  connect(box, SIGNAL(toggled(bool)), this, SLOT(update_configuration(bool)));
	}
      }
      chanBox->setLayout(layout1);
      layout3->addWidget(chanBox); }
    layout3->addLayout(_layout);
    layout3->addStretch();
    layout->addLayout(_layout3=layout3); }
  layout->addWidget(_frame->widget(),1);

  layout->addWidget(_stack);
  connect(_stack, SIGNAL(hidden()), this, SLOT(hide_stack()));

  if (_use_scroll_area) {
    layout->addStretch(2);
    QScrollArea* scroll = new QScrollArea;
    { QWidget* w = new QWidget(0);
      w->setLayout(layout);
      scroll->setWidget(w); }

    scroll->setWidgetResizable(true);
    
    layout = new QHBoxLayout;
    layout->addWidget(scroll);
    setLayout(layout);
  }
  else
    setLayout(layout);

  connect(this, SIGNAL(description_changed(int)), this, SLOT(_read_description(int)));
  connect((AbsClient*)this, SIGNAL(changed()),    this, SLOT(update_configuration()));

  //  _aggchans = new AggChannels(_channels,NCHANNELS);
}

Ami::Qt::Client::~Client() 
{
  if (_manager) delete _manager;
  delete[] _iovload;
  delete _sem;
}

const QString& Ami::Qt::Client::title() const { return _title; }

void Ami::Qt::Client::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  for(unsigned i=0; i<NCHANNELS; i++) 
    XML_insert(p, "ChannelDef", "_channels", _channels[i]->save(p) );

  XML_insert(p, "Display", "_frame", _frame->save(p) );
  XML_insert(p, "Control", "_control", _control->save(p) );
}

void Ami::Qt::Client::load(const char*& p)
{
  for(unsigned i=0; i<NCHANNELS; i++) {
    disconnect(_channels[i], SIGNAL(changed()), (AbsClient*)this, SIGNAL(changed()));
  }

  unsigned nchannels = 0;

  XML_iterate_open(p,tag)
    if (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_channels") {
      _channels[nchannels]->load(p);
      connect(_channels[nchannels], SIGNAL(changed()), (AbsClient*)this, SIGNAL(changed()));
      nchannels++;
    }
    else if (tag.name == "_frame")
      _frame->load(p);
    else if (tag.name == "_control")
      _control->load(p);
  XML_iterate_close(Client,tag);

  printf("Client %s is %s\n",qPrintable(_title),isVisible()?"Visible":"Hidden");
}

void Ami::Qt::Client::reset_plots()
{
  _reset=true;
  update_configuration(); 
}

void Ami::Qt::Client::addWidget(QWidget* w) { _layout->addWidget(w); }

Ami::Qt::Display& Ami::Qt::Client::display() { return *_frame; }

const Ami::Qt::Display& Ami::Qt::Client::display() const { return *_frame; }

void Ami::Qt::Client::connected()
{
  if (_manager) {
    _status->set_state(Status::Connected);
    _manager->discover();
  }
}

void Ami::Qt::Client::discovered(const DiscoveryRx& rx)
{
  _status->set_state(Status::Discovered);
  //  printf("%s Discovered\n",qPrintable(title()));

  //  The EnvClient requests all variable sets
#if 0
  //  iterate through discovery and print
  for(int i=0; i<Ami::NumberOfSets; i++) {
    Ami::ScalarSet set((Ami::ScalarSet)i);
    FeatureRegistry::instance(set).insert(rx.features(set));
  }
#endif

  char channel_name [128]; 
  strcpy(channel_name ,qPrintable(_title));
  if ((_input_entry = rx.entry(channel_name)) &&
      (_input_entry->info()    == this->info) &&
      (_input_entry->channel() == this->channel)) {
    _input = _input_entry->signature();
    //    setWindowModified(!_input_entry->recorded());
    _frame->prototype(_input_entry);
    _prototype(*_input_entry);
    setWindowTitle(QString("%1[*]").arg(_title));
  }
  else {
    _input_entry=0;
    printf("%s [%08x.%08x.%d] not found\n", channel_name, info.phy(), info.log(), channel);

    for(  const DescEntry* e = rx.entries(); e < rx.end(); 
          e = reinterpret_cast<const DescEntry*>
            (reinterpret_cast<const char*>(e) + e->size())) {
      printf("  [%d] %s\n",e->signature(),e->name());
    }
    setWindowTitle(QString("%1 (invalid)[*]").arg(_title));
  }
#ifdef DBUG
  else if (_throttled) {
    printf("%s [%08x.%08x.%d] found while throttled\n", channel_name, info.phy(), info.log(), channel);
  }
#endif

  _manager->configure();
}

unsigned Ami::Qt::Client::_preconfigure(char*&    p,
                                        unsigned  input,
                                        unsigned& output,
                                        ConfigureRequest::Source& source)
{
  return input;
}

int  Ami::Qt::Client::configure       (iovec* iov) 
{
  _status->set_state(Status::Configured);
  //  printf("%s Configure\n",qPrintable(title()));
  if (_input_entry==0) {
    //    printf("input_entry not found\n");
    return 0;
  }
  else {

    int signatures[NCHANNELS];
    for(unsigned i=0; i<NCHANNELS; i++)
      signatures[i] = -1;

    char* p = _request.reset();

    if (_reset) {
      _reset = false;
      ConfigureRequest& req = *new (p) ConfigureRequest(ConfigureRequest::Reset,
							ConfigureRequest::Analysis,
							_input);
      p = _request.extend(req.size());
    }
    else {
      _input_source = ConfigureRequest::Discovery;
      unsigned input = _preconfigure(p,_input,_output_signature, _input_source);
      //
      //  Configure channels which depend upon others
      //
      bool lAdded;
      do {
	lAdded=false;
	for(unsigned i=0; i<NCHANNELS; i++) {
	  if (signatures[i]<0) {
	    int sig = _channels[i]->configure(p,input,_output_signature,
					      _channels.data(),signatures,NCHANNELS,_input_source);
	    if (sig >= 0) {
	      signatures[i] = sig;
	      lAdded = true;
	      //	    printf("Added signature %d for channel %d\n",sig,i);
	    }
	  }
	}
      } while(lAdded);

      char* hp = p = _request.extend(p-_request.base());

      _configure(p,input,_output_signature,
		 _channels.data(),signatures,NCHANNELS);

      if (p==hp && !isVisible()) {  // nothing to show
	return 0;
      }

      _request.extend(p-hp);
    }
    iov[0].iov_base = _request.base();
    iov[0].iov_len  = _request.extent();
    return 1;
  }
}

//
//  This state is unnecessary
//
int  Ami::Qt::Client::configured      () 
{
  //  printf("%s Configured\n",qPrintable(title()));
  return 0; 
}

//
//  read_description adds/removes plots
//
int Ami::Qt::Client::read_description(Socket& socket, int len)
{
  _description.reset  ();
  _description.reserve(len);
  int size = socket.read(_description.base(),len);

  if (size<0) {
    printf("Read error in Ami::Qt::Client::read_description.\n");
    return 0;
  }

  _description.extend(size);

  //  printf("emit description\n");
  emit description_changed(size);

  _sem->take();

  return size;
}

void Ami::Qt::Client::_read_description(int size)
{
  //  printf("%s Described si\n",qPrintable(_title));
  //  printf("description\n"); 

  _cds.reset();

  const char* payload   = _description.base();
  const char* const end = payload + size;
  //  dump(payload, size);

  //  { const Desc* e = reinterpret_cast<const Desc*>(payload);
  //    printf("Cds %s\n",e->name()); }
  payload += sizeof(Desc);

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

  bool show = isVisible();
  for(unsigned i=0; i<NCHANNELS; i++)
    _channels[i]->setup_payload(_cds,show);

  _setup_payload(_cds);

  int n = _cds.totalentries();
  if (n>int(_niovload)) {
    delete[] _iovload;
    _iovload = new iovec[_niovload=n];
  }
  _niovread = _cds.payload(_iovload, _cds.request());

  _status->set_state(Status::Described);

  _sem->give();
}

//
//  read_payload changes plot contents
//
int Ami::Qt::Client::read_payload     (Socket& socket, int size)
{
  int nbytes = 0;
  if (_niovread==0) 
    ;
  else if (_status->state() == Status::Requested) {
    _cds.lock().write_lock();
    nbytes = socket.readv(_iovload,_niovread);
    _cds.lock().write_unlock();
  }
  else if (!_one_shot) {
    //
    //  Add together each server's contribution
    //
    printf("Ami::Qt::Client::read_payload: multiple server processing [state %d, socket %d, size %d]\n",
           _status->state(), socket.socket(), size);
  }
  _status->set_state(Status::Received, nbytes);

  return nbytes;
}

void Ami::Qt::Client::process         () 
{
  //
  //  Perform client-side processing
  //
  _frame  ->update();
  _update();
  
  _status->set_state(Status::Processed);
}

void Ami::Qt::Client::managed(ClientManager& mgr)
{
  _manager = &mgr;
  show();
  _manager->connect();
}

void Ami::Qt::Client::request_payload()
{
  _attempts++;
  if (_status->state() == Status::Described ||
      _status->state() == Status::Processed) {
    _throttled = false;
    _status->set_state(Status::Requested, _cds.request().value());
    _manager->request_payload(_cds.request());
  }
  else if (_status->state() == Status::Requested) {
    _denials++;
    if (!_throttled) {
      _throttled = true;
      _status->set_state(Status::Throttled);
    }
    if ((_denials%20)==1) {
      QString msg = QString("Client %1 request payload throttled %2/%3").arg(_title).arg(_denials).arg(_attempts);
      printf("%s\n",qPrintable(msg));
      _manager->dump_throttle();
      ControlLog::instance().appendText(msg);
#ifdef DBUG
      printf("..resetting throttle.\n");
      _status->set_state(Status::Processed);
#endif
    }
  }
}

void Ami::Qt::Client::one_shot(bool l) 
{
  _one_shot=l; 
}

void Ami::Qt::Client::update_configuration(bool)
{
  update_configuration();
}

void Ami::Qt::Client::update_configuration()
{
  if (_manager)
    _manager->configure();
}

void Ami::Qt::Client::showEvent(QShowEvent* e)
{
  QWidget::showEvent(e);
  if (_status->state() >= Status::Discovered)
    update_configuration();
}

void Ami::Qt::Client::hideEvent(QHideEvent* e)
{
  QWidget::hideEvent(e);
  if (_status->state() >= Status::Discovered)
    update_configuration();
}

void Ami::Qt::Client::set_chrome_visible(bool v)
{
  _chrome_changed = true;
  QtUtils::setChildrenVisible(_layout3,v);
  // QtUtils::setChildrenVisible(_layout ,v);
  _stack->setVisible(v);
  updateGeometry();
  resize(minimumWidth(),minimumHeight());
}

void Ami::Qt::Client::hide_stack()
{
  _chrome_changed = true;
  updateGeometry();
  resize(minimumWidth(),minimumHeight());
}

void Ami::Qt::Client::paintEvent(QPaintEvent* e)
{
  if (_chrome_changed) {
    resize(minimumWidth(),minimumHeight());
    _chrome_changed = false;
  }
  if (_initial_hide) {
    _initial_hide = false;
    setVisible(false);
  }
  QWidget::paintEvent(e);
}

bool Ami::Qt::Client::svc() const { return false; }

void Ami::Qt::Client::use_scroll_area(bool v) { _use_scroll_area=v; }
