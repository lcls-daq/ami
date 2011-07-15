#include "DetectorSelect.hh"

#include "ami/qt/QtTopWidget.hh"
#include "ami/qt/WaveformClient.hh"
#include "ami/qt/ImageClient.hh"
#include "ami/qt/CspadClient.hh"
#include "ami/qt/EnvClient.hh"
#include "ami/qt/TdcClient.hh"
#include "ami/qt/SummaryClient.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/PrintAction.hh"
#include "ami/qt/DetectorSave.hh"
#include "ami/qt/DetectorReset.hh"
#include "ami/qt/DetectorListItem.hh"
#include "ami/qt/Defaults.hh"
#include "ami/qt/FilterSetup.hh"
#include "ami/qt/RateDisplay.hh"
#include "ami/client/ClientManager.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/service/Port.hh"

#include "pdsdata/xtc/BldInfo.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QFont>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtCore/QTimer>

#include <errno.h>

using namespace Ami::Qt;

static const Pds::DetInfo envInfo(0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::Evr,0);
static const Pds::DetInfo noInfo (0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::NoDevice,0);

static const int MaxConfigSize = 0x100000;
static const int BufferSize = 0x8000;

static void insertInfo(char*& p, const Pds::DetInfo& info, unsigned channel)
{
  QtPersistent::insert(p,(unsigned)info.detector());
  QtPersistent::insert(p,(unsigned)info.detId());
  QtPersistent::insert(p,(unsigned)info.device());
  QtPersistent::insert(p,(unsigned)info.devId());
  QtPersistent::insert(p,(unsigned)channel);
}


static void extractInfo(const char*& p, Pds::DetInfo& info, unsigned& channel) 
{
  unsigned det   = QtPersistent::extract_i(p);
  unsigned detId = QtPersistent::extract_i(p);
  unsigned dev   = QtPersistent::extract_i(p);
  unsigned devId = QtPersistent::extract_i(p);
  info = Pds::DetInfo(0,
		      Pds::DetInfo::Detector(det), detId,
		      Pds::DetInfo::Device  (dev), devId);
  channel = QtPersistent::extract_i(p);
}


DetectorSelect::DetectorSelect(const QString& label,
			       unsigned ppinterface,
			       unsigned interface,
			       unsigned serverGroup) :
  QtPWidget   (0),
  _ppinterface(ppinterface),
  _interface  (interface),
  _serverGroup(serverGroup),
  _clientPort (Port::clientPortBase()),
  _manager    (new ClientManager(ppinterface,
				 interface, serverGroup, 
				 _clientPort++,*this)),
  _filters    (new FilterSetup(*_manager)),
  _request    (new char[BufferSize]),
  _rate_display(new RateDisplay(_manager)),
  _sem        (Semaphore::EMPTY)
{
  setWindowTitle(label);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QVBoxLayout* l = new QVBoxLayout;
  { QLabel* title = new QLabel(label);
    title->setWordWrap(true);
    QFont font = title->font();
    font.setPointSize(font.pointSize()+8);
    title->setFont(font);
    l->addWidget(title,0,::Qt::AlignHCenter); }
  { QGroupBox* setup_box = new QGroupBox("Setup");
    QVBoxLayout* layout = new QVBoxLayout;
    QPushButton* saveB    = new QPushButton("Save");
    QPushButton* loadB    = new QPushButton("Load");
    QPushButton* defaultB = new QPushButton("Defaults");
    QPushButton* printB   = new QPushButton("Printer");
    layout->addWidget(saveB);
    layout->addWidget(loadB);
    layout->addWidget(defaultB);
    //    layout->addWidget(printB);
    connect(saveB   , SIGNAL(clicked()), this, SLOT(save_setup()));
    connect(loadB   , SIGNAL(clicked()), this, SLOT(load_setup()));
    connect(printB  , SIGNAL(clicked()), this, SLOT(print_setup()));
    connect(defaultB, SIGNAL(clicked()), this, SLOT(default_setup()));
    setup_box->setLayout(layout);
    l->addWidget(setup_box); }
  { QGroupBox* data_box  = new QGroupBox("Data");
    QVBoxLayout* layout = new QVBoxLayout;    

    QPushButton* resetB  = new QPushButton("Reset Plots");
    QPushButton* saveB   = new QPushButton("Save Plots");
    QPushButton* filterB = new QPushButton("Event Filter"); 
    layout->addWidget(resetB);
    layout->addWidget(saveB);
    layout->addWidget(filterB);
    connect(resetB , SIGNAL(clicked()), this, SLOT(reset_plots()));
    connect(saveB  , SIGNAL(clicked()), this, SLOT(save_plots()));
    connect(filterB, SIGNAL(clicked()), this, SLOT(set_filters()));

    layout->addWidget(_detList = new QListWidget(this));
    *new DetectorListItem(_detList, "Env"    , envInfo, 0);
    *new DetectorListItem(_detList, "Summary", noInfo , 0);
    *new DetectorListItem(_detList, "User",    noInfo , 1);

    connect(_detList, SIGNAL(itemClicked(QListWidgetItem*)), 
	    this, SLOT(show_detector(QListWidgetItem*)));

    data_box->setLayout(layout);
    l->addWidget(data_box); }
  _rate_display->addLayout(l);
  setLayout(l);

  connect(this, SIGNAL(detectors_discovered(const char*)), this, SLOT(change_detectors(const char*)));

  //  autoload();

  _autosave_timer = new QTimer(this);
  _autosave_timer->setSingleShot(true);
  connect(_autosave_timer, SIGNAL(timeout()), this, SLOT(autosave()));

#if 0
  {
    timespec ts;
    ts.tv_sec = 3;
    ts.tv_nsec = 0;
    nanosleep(&ts, 0);
  }
#endif
  _manager->connect();
}

DetectorSelect::~DetectorSelect()
{
  for(std::list<QtTopWidget*>::iterator it = _client.begin();
      it != _client.end(); it++)
    if (*it) {
      disconnect((*it), SIGNAL(changed()), this, SLOT(queue_autosave()));
      delete (*it);
    }

  delete _filters;
  delete _manager;
  delete[] _request;
}

int DetectorSelect::get_setup(char* buffer) const
{
  char* p = buffer;

  for(std::list<QtTopWidget*>::const_iterator it = _client.begin();
      it != _client.end(); it++)
    if ((*it)) {
      QtPersistent::insert(p,QString("DetectorSelect"));
      insertInfo(p, (*it)->info, (*it)->channel);
      QtPersistent::insert(p,QString((*it)->title()));
      (*it)->save(p);
    }
  QtPersistent::insert(p,QString("EndDetectorSelect"));

  save(p);
  //  _save_box ->save(p);
  //  _reset_box->save(p);

  return p-buffer;
}

void DetectorSelect::save_setup ()
{
  char* buffer = new char[MaxConfigSize];

  int len = get_setup(buffer);
  if (len > MaxConfigSize) {
    printf("DetectorSelect::save_setup exceeded save buffer size (0x%x/0x%x)\n",
	   len, MaxConfigSize);
  }

  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def = QString("%1/%2.ami").arg(Path::base()).arg(time_buffer);
  QString fname =     
    QFileDialog::getSaveFileName(this,"Save Setup to File (.ami)",
                                 def,"*.ami");
  FILE* o = fopen(qPrintable(fname),"w");
  if (o) {
    fwrite(buffer,len,1,o);
    fclose(o);
    printf("Saved %d bytes to %s\n",len,qPrintable(fname));
  }
  else {
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    QMessageBox::critical(this,"Save Error",msg);
  }

  delete[] buffer;
}

void DetectorSelect::load_setup ()
{
  // get the file 
  QString fname = QFileDialog::getOpenFileName(this,"Load Setup from File (.ami)",
					       Path::base(), "*.ami");

  _load_setup_from_file(qPrintable(fname));
}

void DetectorSelect::_load_setup_from_file(const char* fname)
{
  FILE* f = fopen(fname,"r");
  if (!f) {
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    QMessageBox::critical(this,"Load Error",msg);
    return;
  }
  char* buffer = new char[MaxConfigSize];
  int size = fread(buffer,1,MaxConfigSize,f);
  fclose(f);

  printf("Load %d bytes from %s @ %p\n",size,fname,buffer);

  set_setup(buffer, size);

  delete[] buffer;
}

void DetectorSelect::set_setup(const char* p, int size)
{
  // parse the input
  //
  //  Create the clients, if necessary, load them, and connect
  //
  QString name = QtPersistent::extract_s(p);
  Pds::DetInfo info;
  unsigned     channel;

  while(name==QString("DetectorSelect")) {
    extractInfo(p, info, channel);
    name = QtPersistent::extract_s(p);
    printf("Seeking %s\n",qPrintable(name));

    bool lFound=false;
    for(std::list<QtTopWidget*>::iterator it = _client.begin();
	it != _client.end(); it++)
      if ((*it)->info == info && (*it)->channel == channel) {
	printf("Loading %s\n",qPrintable(name));
	lFound=true;
	(*it)->load(p);
	break;
      }

    if (!lFound) {
      Ami::Qt::AbsClient* c = _create_client(info,channel);
      if (c) {
	c->load(p);
	_connect_client(c);
      }
    }

    name=QtPersistent::extract_s(p);
  }

  load(p);
  //  _save_box ->load(p);
  //  _reset_box->load(p);
}

void DetectorSelect::print_setup()
{
  QPrintDialog* d = new QPrintDialog(PrintAction::printer(),this);
  d->exec();
  delete d;
}

void DetectorSelect::default_setup()
{
  Defaults::instance()->show();
}

void DetectorSelect::set_filters()
{
  _filters->show();
}

void DetectorSelect::reset_plots()
{
  // _reset_box->apply();

  for(std::list<QtTopWidget*>::iterator it=_client.begin();
      it!=_client.end(); it++)
    (*it)->reset_plots();
}

void DetectorSelect::save_plots()
{
  // _save_box->apply();

  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def = QString("%1/%2").arg(Path::base()).arg(time_buffer);

  QString prefix = QFileDialog::getSaveFileName(0,"Save Files with Prefix",
						def,"*.dat");
  for(std::list<QtTopWidget*>::iterator it=_client.begin();
      it!=_client.end(); it++)
    (*it)->save_plots(QString("%1_%2").arg(prefix).arg((*it)->title()));
}

Ami::Qt::AbsClient* DetectorSelect::_create_client(const Pds::DetInfo& info, 
						   unsigned channel)
{
  Ami::Qt::AbsClient* client = 0;
  if (info.level()==Pds::Level::Source) {
    switch(info.device()) {
    case Pds::DetInfo::NoDevice : 
      if (channel==0) client = new Ami::Qt::SummaryClient (this, noInfo , channel, "Summary", ConfigureRequest::Summary); 
      else            client = new Ami::Qt::SummaryClient (this, noInfo , channel, "User"   , ConfigureRequest::User); 
      break;
    case Pds::DetInfo::Evr      : client = new Ami::Qt::EnvClient     (this, envInfo, 0); break;
    case Pds::DetInfo::Acqiris  : client = new Ami::Qt::WaveformClient(this, info, channel); break;
    case Pds::DetInfo::AcqTDC   : client = new Ami::Qt::TdcClient     (this, info, channel); break;
    case Pds::DetInfo::Opal1000 : 
    case Pds::DetInfo::TM6740   : 
    case Pds::DetInfo::pnCCD    :
    case Pds::DetInfo::Princeton: 
    case Pds::DetInfo::Fccd     : client = new Ami::Qt::ImageClient   (this, info, channel); break;
    case Pds::DetInfo::Cspad    : client = new Ami::Qt::CspadClient   (this, info, channel); break;
    default: printf("Device type %x not recognized\n", info.device()); break;
    }
  }
  else if (info.level()==Pds::Level::Reporter) {
    const Pds::BldInfo& bld = reinterpret_cast<const Pds::BldInfo&>(info);
    switch(bld.type()) {
    case Pds::BldInfo::HxxDg1Cam:
    case Pds::BldInfo::HfxDg2Cam:
    case Pds::BldInfo::HfxDg3Cam:
    case Pds::BldInfo::XcsDg3Cam:
    case Pds::BldInfo::HfxMonCam:
      client = new Ami::Qt::ImageClient(this, info, channel); break;
    default: 
      printf("Bld type %x not recognized\n", bld.type()); break;
    }
  }
  return client;
 }

 void DetectorSelect::_connect_client(Ami::Qt::AbsClient* client)
 {
   ClientManager* manager = new ClientManager(_ppinterface,
					      _interface,     
					      _serverGroup,
					      _clientPort++,
					      *client);	
   client->managed(*manager);					
   _client.push_back(client);
   //   _update_groups();

   connect(client, SIGNAL(changed()), this, SLOT(queue_autosave()));
}
   
void DetectorSelect::show_detector(QListWidgetItem* item)
{
  DetectorListItem* ditem = static_cast<DetectorListItem*>(item);
  for(std::list<QtTopWidget*>::iterator it = _client.begin(); it != _client.end(); it++)
    if ((*it)->info==ditem->info && (*it)->channel==ditem->channel) {
      (*it)->show();
      return;
    }
  Ami::Qt::AbsClient* c = _create_client(ditem->info,ditem->channel);
  if (c)
    _connect_client(c);
}

void DetectorSelect::connected       () 
{
  _manager->discover(); 
}

int  DetectorSelect::configure       (iovec* iov) 
{ 
  char* p = _request;
  { ConfigureRequest& r = 
      *new(p) ConfigureRequest(ConfigureRequest::Filter,
                               _filters->selected());
    p += r.size(); }

  _rate_display->configure(p);

  iov[0].iov_base = _request;
  iov[0].iov_len  = p-_request;
  return 1; 
}

int  DetectorSelect::configured      () { return 0; }

void DetectorSelect::discovered      (const DiscoveryRx& rx) 
{ 
  _rate_display->discovered(rx);
  emit detectors_discovered(reinterpret_cast<const char*>(&rx));
  _sem.take();
  _manager->configure();
}

void DetectorSelect::change_detectors(const char* c)
{
  const DiscoveryRx& rx = *reinterpret_cast<const DiscoveryRx*>(c);

  setUpdatesEnabled(false);

  //  Remove all clients not discovered
  {
    std::list<QtTopWidget*> remove;
    for(std::list<QtTopWidget*>::iterator it = _client.begin(); it != _client.end(); it++) {
      
      bool lFound=false;
      for(const Ami::DescEntry* e = rx.entries(); e < rx.end();
	  e = reinterpret_cast<const Ami::DescEntry*>
	    (reinterpret_cast<const char*>(e) + e->size()))
	if (((*it)->info == e->info() && (*it)->channel == e->channel()) ||
	    ((*it)->info == envInfo   && (*it)->channel == 0)) {
	  lFound=true;
	  break;
	}
      if (!lFound)
	remove.push_back(*it);
    }
    for(std::list<QtTopWidget*>::iterator it = remove.begin(); it != remove.end(); it++) {
      printf("DetectorSelect::change_detectors removing %s\n",qPrintable((*it)->title()));
      _client.remove(*it);
    }
  }

  //  Register new buttons
  {
    _detList->clear();

    new DetectorListItem(_detList, "Env"    , envInfo, 0);
    new DetectorListItem(_detList, "Summary", noInfo , 0);
    new DetectorListItem(_detList, "User"   , noInfo , 1);

    const Pds::DetInfo noInfo;
    const Ami::DescEntry* n;
    for(const Ami::DescEntry* e = rx.entries(); e < rx.end(); e = n) {
      n = reinterpret_cast<const Ami::DescEntry*>
	(reinterpret_cast<const char*>(e) + e->size());

      printf("Discovered %s [%08x.%08x]\n",e->name(),e->info().log(),e->info().phy());

      if (e->info() == noInfo) {  // Skip unknown devices 
        printf("\tSkip.\n");
	continue;
      }

      new DetectorListItem(_detList, e->name(), e->info(), e->channel());
    }
    _detList->sortItems();
  }

  //  Register filters
  _filters->update(rx);

  //  _update_groups();

  setUpdatesEnabled(true);

  _sem.give();
}

void DetectorSelect::read_description(Socket& s,int len) {
  _rate_display->read_description(s,len);
}

void DetectorSelect::read_payload    (Socket& s,int len) {
  _rate_display->read_payload(s,len);
}

void DetectorSelect::process         () {
  _rate_display->process();
}

/*
void DetectorSelect::_update_groups()
{
  DetectorReset* reset_box = new DetectorReset(*_reset_box);
  delete _reset_box;
  _reset_box = reset_box;
  DetectorSave * save_box  = new DetectorSave (*_save_box);
  delete _save_box;
  _save_box = save_box;
}
*/

void DetectorSelect::queue_autosave()
{
  _autosave_timer->stop();
  _autosave_timer->start(10000);
}

void DetectorSelect::autosave()
{
  static bool warned = false;

  char* buffer = new char[MaxConfigSize];

  int len = get_setup(buffer);

  QString fname = QString("%1/AUTOSAVE.ami").arg(Path::base());

  FILE* o = fopen(qPrintable(fname),"w");
  if (o) {
    fwrite(buffer,len,1,o);
    fclose(o);
    printf("Saved %d bytes to %s\n",len,qPrintable(fname));
  }
  else if (!warned) {
    warned = true;
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    QMessageBox::critical(this,"Save Error",msg);
  }

  delete[] buffer;
}

void DetectorSelect::autoload()
{
  QString fname = QString("%1/AUTOSAVE.ami").arg(Path::base());
  _load_setup_from_file(qPrintable(fname));
}
