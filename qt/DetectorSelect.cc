#include "DetectorSelect.hh"

#include "ami/qt/QtTopWidget.hh"
#include "ami/qt/WaveformClient.hh"
#include "ami/qt/ImageClient.hh"
#include "ami/qt/CspadClient.hh"
#include "ami/qt/EpixClient.hh"
#include "ami/qt/FccdClient.hh"
#include "ami/qt/FrameClient.hh"
#include "ami/qt/PnccdClient.hh"
#include "ami/qt/JungfrauClient.hh"
#include "ami/qt/EnvClient.hh"
#include "ami/qt/LineFitClient.hh"
#include "ami/qt/TdcClient.hh"
#include "ami/qt/SummaryClient.hh"
//#include "ami/qt/ScriptClient.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/PrintAction.hh"
#include "ami/qt/DetectorListItem.hh"
#include "ami/qt/Defaults.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/FilterSetup.hh"
#include "ami/qt/L3Features.hh"
#include "ami/qt/RateDisplay.hh"
#include "ami/qt/PWidgetManager.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/SMPRegistry.hh"
#include "ami/qt/ControlLog.hh"
#include "ami/client/ClientManager.hh"
#include "ami/service/ConnectionManager.hh"
#include "ami/service/Task.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/ConfigureRequest.hh"

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
#include <time.h>
#include <sys/stat.h>

using namespace Ami::Qt;

static const Pds::DetInfo noInfo    (0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::NoDevice,0);
static const Pds::DetInfo envInfo   (0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::NoDevice,1);
static const Pds::DetInfo fitInfo   (0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::NoDevice,2);

static const int MaxConfigSize = 0x100000;
static const int BufferSize = 0x8000;

//
//  Really need to replace DetInfo with Src in all monitoring usage
//  Until then, add level to the match criteria
//
static bool match(const Pds::Src& a,
                  const Pds::Src& b)
{
  return a.level()==b.level() && a.phy()==b.phy();
}

class SrcV : public Pds::Src {
public:
  SrcV(uint32_t l, uint32_t p) { _log=l; _phy=p; }
};


DetectorSelect::DetectorSelect(const QString& label,
                               unsigned ppinterface,
                               unsigned interface,
                               unsigned serverGroup,
                               QWidget* guestBox,
                               bool quiet) :
  QtPWidget   (0),
  //  _quiet      (quiet),
  _quiet      (true),
  _interface  (interface),
  _serverGroup(serverGroup),
  _connect_mgr(new ConnectionManager(ppinterface)),
  _manager    (new ClientManager(interface,
                                 serverGroup,
                                 *_connect_mgr,
                                 *this)),
  _filters    (new FilterSetup(*_manager)),
  _request    (BufferSize),
  _rate_display(new RateDisplay(*_connect_mgr,
                                _manager)),
  _discovered(false),
  _filter_export(new Filter((QtPWidget*)0,"L3T Export", new L3Features)),
  _l3t_export(false),
  _l3t_export_file(0),
  _dump(interface)
{
  _dump.add(*_manager);

  pthread_mutex_init(&_mutex, NULL);
  pthread_cond_init(&_condition, NULL);
  setWindowTitle(label);
  //  setAttribute(::Qt::WA_DeleteOnClose, false);

  QVBoxLayout* l = new QVBoxLayout;

  // Caller can add extra controls here.
  if (guestBox) {
    l->addWidget(guestBox);
  }

  { QGroupBox* setup_box = new QGroupBox("Setup");
    QHBoxLayout* layout = new QHBoxLayout;
    QPushButton* saveB    = new QPushButton("Save");
    QPushButton* loadB    = new QPushButton("Load");
    QPushButton* defaultB = new QPushButton("Defaults");
    //    QPushButton* printB   = new QPushButton("Printer");
    //    QPushButton* testB    = new QPushButton("Test");
    layout->addWidget(saveB);
    layout->addWidget(loadB);
    layout->addWidget(defaultB);
    //    layout->addWidget(testB);
    //    layout->addWidget(printB);
    connect(saveB   , SIGNAL(clicked()), this, SLOT(save_setup()));
    connect(loadB   , SIGNAL(clicked()), this, SLOT(load_setup()));
    //    connect(printB  , SIGNAL(clicked()), this, SLOT(print_setup()));
    connect(defaultB, SIGNAL(clicked()), this, SLOT(default_setup()));
    setup_box->setLayout(layout);
    l->addWidget(setup_box); }
  { QGroupBox* data_box  = new QGroupBox("Data");
    QGridLayout* layout = new QGridLayout;

    QPushButton* resetB  = new QPushButton("Reset Plots");
    QPushButton* saveB   = new QPushButton("Save Plots");
    QPushButton* filterB = new QPushButton("Event Filter");
    QPushButton* exportB = new QPushButton("L3T Export");
    layout->addWidget(resetB ,0,0);
    layout->addWidget(saveB  ,0,1);
    layout->addWidget(filterB,1,0);
    layout->addWidget(exportB,1,1);
    connect(resetB , SIGNAL(clicked()), this, SLOT(reset_plots()));
    connect(this   , SIGNAL(_reset_plots()), this, SLOT(reset_plots()));
    connect(saveB  , SIGNAL(clicked()), this, SLOT(save_plots()));
    connect(filterB, SIGNAL(clicked()), this, SLOT(set_filters()));
    connect(exportB, SIGNAL(clicked()), _filter_export, SLOT(front()));
    connect(_filter_export, SIGNAL(changed()), this, SLOT(l3t_export()));

    layout->addWidget(_detList = new QListWidget(this),2,0,1,2);
    //
    //  The EnvClient is needed at all times to request the PostAnalysis variable set and generate those plots
    //
    { DetectorListItem* ditem = new DetectorListItem(_detList, "LineFit", fitInfo, 2);
      const char* p=0;
      _create_client(ditem->info,ditem->channel,ditem->text(),p)->hide();
    }
    { DetectorListItem* ditem = new DetectorListItem(_detList, "Env"    , envInfo, 1);
      const char* p=0;
      _create_client(ditem->info,ditem->channel,ditem->text(),p);
    }

    connect(_detList, SIGNAL(itemClicked(QListWidgetItem*)),
      this, SLOT(show_detector(QListWidgetItem*)));

    data_box->setLayout(layout);
    l->addWidget(data_box); }
  l->addWidget(&ControlLog::instance(),1);
  _rate_display->addLayout(l);

  l->addWidget(new PWidgetManager);

  setLayout(l);

  connect(this, SIGNAL(detectors_discovered(const char*)), this, SLOT(change_detectors(const char*)));

  //  autoload();

  { QString fname = QString("%1/.defaults.ami").arg(getenv("HOME"));
    FILE* f = fopen(qPrintable(fname),"r");
    if (f) {
      char* buffer = new char[MaxConfigSize];
      int size;
      if ((size=fread(buffer,1,MaxConfigSize,f))>0) {
        sprintf(buffer+size,"</Document>");
        const char* p = buffer;
        XML_iterate_open(p,tag)
          if (tag.element == "Defaults")
            Defaults::instance()->load(p);
          else if (tag.element == "QtTree")
            QtTree::load_preferences(p);
        XML_iterate_close(DetectorSelect,tag);
      }
      fclose(f);
      delete[] buffer;
    }
  }
  { QString fname = QString("%1/.ami_favorites").arg(getenv("HOME"));
    FILE* f = fopen(qPrintable(fname),"r");
    if (f) {
      char* buffer = new char[MaxConfigSize];
      int size;
      if ((size=fread(buffer,1,MaxConfigSize,f))>0) {
        buffer[size]=0;
        QtTree::load_favorites(buffer);
      }
      delete[] buffer;
    }
  }

  _autosave_timer = new QTimer(this);
  _autosave_timer->setSingleShot(true);
  connect(_autosave_timer, SIGNAL(timeout()), this, SLOT(autosave()));

  _snapshot_timer = new QTimer(this);
  _snapshot_timer->setSingleShot(false);
  connect(_snapshot_timer, SIGNAL(timeout()), this, SLOT(run_snapshot()));

  _snapshot_timer->start(900000); // 15 minutes

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

  delete _rate_display;
  delete _filters;
  delete _filter_export;
  delete _manager;
}

int DetectorSelect::get_setup(char* buffer) const
{
  char id[64];

  char* p = buffer;

  XML_insert( p, "FilterSetup", "_filters", _filters->save(p) );

  for(std::list<QtTopWidget*>::const_iterator it = _client.begin();
      it != _client.end(); it++)
    if ((*it)) {
      sprintf(id, "%08x.%08x.%d", (*it)->info.log(), (*it)->info.phy(), (*it)->channel);
      XML_insert( p, id, qPrintable((*it)->title()), (*it)->save(p) );
    }

  XML_insert( p, "Filter", "_filter_export", _filter_export->save(p) );

  XML_insert( p, "QtTopWidget", "self", save(p) );

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

  Path::saveAmiFile(this, buffer, len);

  delete[] buffer;
}

void DetectorSelect::load_setup ()
{
  // get the file
  QString fname = QFileDialog::getOpenFileName(this,"Load Setup from File (.ami)",
                 Path::base(), "*.ami");

  load_setup(qPrintable(fname));
}

void DetectorSelect::load_setup(const char* fname)
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

  //
  //  Terminate the file
  //
  size += sprintf(buffer+size,"</Document>");

  set_setup(buffer, size);

  delete[] buffer;
}

void DetectorSelect::set_setup(const char* p, int size)
{
  const char* start = p;
  //
  // parse the input
  //
  //  Create the clients, if necessary, load them, and connect
  //
  for(Ami::XML::TagIterator it(p); !it.end(); it++) {
    const Ami::XML::StartTag& tag = *it;

    printf("DetectorSelect tag %s/%s [%d]\n",
           tag.element.c_str(),tag.name.c_str(),(int)(p-start));

    if (tag.name == "self")
      load(p);
    else if (tag.name == "_filters")
      _filters->load(p);
    else if (tag.name == "_filter_export")
      _filter_export->load(p);
    else {

      uint32_t log, phy, channel;
      if (sscanf(tag.element.c_str(),"%08x.%08x.%d",&log,&phy,&channel)!=3) {
        log = 0x01000000;
        sscanf(tag.element.c_str(),"%08x.%d",&phy,&channel);
      }

      printf("Seeking %s (%08x.%08x.%d)\n",tag.name.c_str(),log,phy,channel);

      SrcV info(log,phy);

      bool lFound=false;
      for(std::list<QtTopWidget*>::iterator it = _client.begin();
          it != _client.end(); it++)
        if (match((*it)->info,info) && (*it)->channel == channel) {
          printf("Loading %s\n",tag.name.c_str());
          lFound=true;
          (*it)->load(p);
          break;
        }

      if (!lFound) {
        _create_client(info,channel,tag.name.c_str(),p);
      }
    }
  }

  //  emit _reset_plots();
}

void DetectorSelect::print_setup()
{
  QPrintDialog* d = new QPrintDialog(PrintAction::printer(),this);
  d->exec();
  delete d;
}

void DetectorSelect::default_setup()
{
  Defaults::instance()->front();
}

void DetectorSelect::set_filters()
{
  _filters->show();
}

void DetectorSelect::l3t_export()
{
  _l3t_export=true;
  _manager->configure();
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

Ami::Qt::AbsClient* DetectorSelect::_create_client(const Pds::Src& src,
						   unsigned channel,
                                                   const QString& name,
                                                   const char*& p)
{
  const Pds::DetInfo& info = static_cast<const Pds::DetInfo&>(src);

  Ami::Qt::AbsClient* client = 0;
  if (info.level()==Pds::Level::Source) {
    switch(info.device()) {
    case Pds::DetInfo::NoDevice :
      switch(info.devId()) {
      case 0: client = new Ami::Qt::SummaryClient(this, info , channel, "Summary", ConfigureRequest::Summary); break;
      case 1: client = new Ami::Qt::EnvClient    (this, info, channel, name); break;
      case 2: client = new Ami::Qt::LineFitClient(this, info, channel, name); break;
      default: break; }
      break;
    case Pds::DetInfo::OceanOptics :
    case Pds::DetInfo::Imp      :
    case Pds::DetInfo::Wave8    :
    case Pds::DetInfo::EpixSampler:
    case Pds::DetInfo::Acqiris  : client = new Ami::Qt::WaveformClient(this, info, channel, name); break;
    case Pds::DetInfo::AcqTDC   : client = new Ami::Qt::TdcClient     (this, info, channel, name); break;
    case Pds::DetInfo::Timepix  :
    case Pds::DetInfo::Princeton:
    case Pds::DetInfo::Fli      :
    case Pds::DetInfo::Pimax    :
    case Pds::DetInfo::Pixis    :
      client = new Ami::Qt::ImageClient   (this, info, channel, name);
      break;
    case Pds::DetInfo::Andor    :
    case Pds::DetInfo::DualAndor:
    case Pds::DetInfo::Zyla     :
    case Pds::DetInfo::Opal1000 :
    case Pds::DetInfo::Opal2000 :
    case Pds::DetInfo::Opal4000 :
    case Pds::DetInfo::Opal8000 :
    case Pds::DetInfo::OrcaFl40 :
    case Pds::DetInfo::Phasics  :
    case Pds::DetInfo::Quartz4A150 :
    case Pds::DetInfo::Rayonix  :
    case Pds::DetInfo::TM6740   :
    case Pds::DetInfo::ControlsCamera :
    case Pds::DetInfo::Uxi :
    case Pds::DetInfo::Archon :
    case Pds::DetInfo::StreakC7700:
      client = new Ami::Qt::FrameClient   (this, info, channel, name);
      break;
    case Pds::DetInfo::Jungfrau : client = new Ami::Qt::JungfrauClient(this, info, channel, name); break;
    case Pds::DetInfo::Fccd     : client = new Ami::Qt::FccdClient    (this, info, channel, name); break;
    case Pds::DetInfo::Cspad    :
    case Pds::DetInfo::Cspad2x2 : client = new Ami::Qt::CspadClient   (this, info, channel, name); break;
    case Pds::DetInfo::pnCCD    : client = new Ami::Qt::PnccdClient   (this, info, channel, name); break;
    case Pds::DetInfo::Epix     :
    case Pds::DetInfo::Epix100a :
    case Pds::DetInfo::Epix10k  :
    case Pds::DetInfo::Epix10ka :
    case Pds::DetInfo::EpixS    :
    case Pds::DetInfo::Epix10ka2M   :
    case Pds::DetInfo::Epix10kaQuad :
      if (channel==0)
	client = new Ami::Qt::EpixClient    (this, info, channel, name);
      else
	client = new Ami::Qt::WaveformClient(this, info, channel, name); 
      break;
    case Pds::DetInfo::Fccd960  :
    case Pds::DetInfo::NumDevice: client = new Ami::Qt::EpixClient    (this, info, channel, name); break;
    default: printf("Device type %x not recognized\n", info.device()); break;
    }
  }
  else if (src.level()==Pds::Level::Reporter) {
    const Pds::BldInfo& bld = reinterpret_cast<const Pds::BldInfo&>(src);
    switch(bld.type()) {
    case Pds::BldInfo::HxxDg1Cam:
    case Pds::BldInfo::HfxDg2Cam:
    case Pds::BldInfo::HfxDg3Cam:
    case Pds::BldInfo::XcsDg3Cam:
    case Pds::BldInfo::HfxMonCam:
    case Pds::BldInfo::CxiDg1Pim:
    case Pds::BldInfo::CxiDg2Pim:
    case Pds::BldInfo::CxiDg3Spec:
    case Pds::BldInfo::CxiDg3Pim:
      client = new Ami::Qt::ImageClient(this, info, channel, name); break;
    case Pds::BldInfo::FeeSpec0:
    case Pds::BldInfo::SxrSpec0:
    case Pds::BldInfo::XppSpec0:
      client = new Ami::Qt::WaveformClient(this, info, channel, name); break;
    default:
      printf("Bld type %x not recognized\n", bld.type()); break;
    }
  }
  else if (src.level()==Pds::Level::Event) {
    client = new Ami::Qt::SummaryClient (this, info , channel, name, ConfigureRequest::User);
  }
  else {
    printf("Ignoring %s [%08x.%08x]\n",qPrintable(name), src.log(), src.phy());
  }

  if (client) {
    if (p) client->load(p);
    _connect_client(client);
  }

  return client;
 }

 void DetectorSelect::_connect_client(Ami::Qt::AbsClient* client)
 {
   ClientManager* manager = new ClientManager(_interface,
                                              _serverGroup,
                                              *_connect_mgr,
                                              *client);
   client->managed(*manager);
   _client.push_back(client);
   //   _update_groups();

   connect(client, SIGNAL(changed()), this, SLOT(queue_autosave()));

  _dump.add(*manager);
}

void DetectorSelect::show_detector(QListWidgetItem* item)
{
  DetectorListItem* ditem = static_cast<DetectorListItem*>(item);
  for(std::list<QtTopWidget*>::iterator it = _client.begin(); it != _client.end(); it++) {
    if (match((*it)->info,ditem->info) && (*it)->channel==ditem->channel) {
      (*it)->front();
      return;
    }
  }
  const char* p=0;
  _create_client(ditem->info,ditem->channel,ditem->text(),p);
}

void DetectorSelect::connected       ()
{
  _manager->discover();
}

int  DetectorSelect::configure       (iovec* iov)
{
  char* p = _request.reset();
  { ConfigureRequest& r =
      *new(p) ConfigureRequest(ConfigureRequest::Filter,
                               _filters->selected(),
			       _filters->filter());
    p = _request.extend(r.size()); }

  if (_l3t_export) {
    _l3t_export=false;

    if (!_l3t_export_file) {
      _l3t_export_file = new QFileDialog(_filter_export);
      _l3t_export_file->setFileMode(QFileDialog::AnyFile);
      _l3t_export_file->setNameFilter("*.l3t");
      _l3t_export_file->setDefaultSuffix("l3t");
      _l3t_export_file->selectFile(QString("%1/ami.l3t").arg(getenv("HOME")));
    }

    if (_l3t_export_file->exec()) {
      QStringList ofiles = _l3t_export_file->selectedFiles();

      if (ofiles.length()>0) {
	ConfigureRequest& r =
	  *new(p) ConfigureRequest(ConfigureRequest::Filter,
				   (1<<31),
				   *_filter_export->filter(),
				   qPrintable(ofiles[0]));
	p = _request.extend(r.size());
      }
    }
  }

  _rate_display->configure(p);
  _request.extend(p);

  iov[0].iov_base = _request.base();
  iov[0].iov_len  = _request.extent();
  return 1;
}

int  DetectorSelect::configured      () { return 0; }

void DetectorSelect::discovered      (const DiscoveryRx& rx)
{
  SMPRegistry::instance().nservers(rx.nsources());

  for(int i=0; i<Ami::NumberOfSets; i++) {
    Ami::ScalarSet set((Ami::ScalarSet)i);
    FeatureRegistry::instance(set).insert(rx.features(set));
  }

  _rate_display->discovered(rx);

  pthread_mutex_lock(&_mutex);
  _discovered = false;
  emit detectors_discovered(reinterpret_cast<const char*>(&rx));
  for (;;) {
    if (_discovered) {
      break;
    }
    pthread_cond_wait(&_condition, &_mutex);
  }
  pthread_mutex_unlock(&_mutex);

  _manager->configure();
}

void DetectorSelect::beginRun(unsigned v) { printf("BeginRun %u\n",v); }
void DetectorSelect::endRun  (unsigned v) { printf("EndRun %u\n",v); }

void DetectorSelect::change_detectors(const char* c)
{
  const DiscoveryRx& rx = *reinterpret_cast<const DiscoveryRx*>(c);

  setUpdatesEnabled(false);

  //  Register new buttons
  {
    _detList->clear();

    new DetectorListItem(_detList, "Env"    , envInfo, 1);
    new DetectorListItem(_detList, "LineFit", fitInfo, 2);

    const Ami::DescEntry* n = 0;
    const Ami::DescEntry* rx_entries = rx.entries();
    const Ami::DescEntry* rx_end = rx.end();
    for (const Ami::DescEntry* e = rx_entries; e < rx_end; e = n) {
      int e_size = e->size();
      n = reinterpret_cast<const Ami::DescEntry*> (reinterpret_cast<const char*>(e) + e_size);
      if (! _quiet) {
        printf("Discovered %s [%08x.%08x] size=%d\n",e->name(),e->info().log(),e->info().phy(),e->size());
      }
      if (e->info().level() == Pds::Level::Control) {
        if (! _quiet) {
          printf("\tSkip.\n");
        }
      } else {
        (new DetectorListItem(_detList, e->name(), e->info(), e->channel()))
          ->setState(e->recorded() ? DetectorListItem::OK : DetectorListItem::Warning);
      }
      if (e->size() == 0) {
        printf("Stopped discovering because size is 0 for %s [%08x.%08x]\n",e->name(),e->info().log(),e->info().phy());
        break;
      }
    }
    _detList->sortItems();
  }

  //
  //  Remove clients not in detector list
  //
  std::list<QtTopWidget*> keepList;
  for(std::list<QtTopWidget*>::iterator it = _client.begin(); it != _client.end(); it++) {
    bool lFound=false;
    for(int i=0; i<_detList->count(); i++) {
      DetectorListItem* ditem = static_cast<DetectorListItem*>(_detList->item(i));
      if (match((*it)->info,ditem->info) && 
          (*it)->channel==ditem->channel &&
          (*it)->title()==ditem->text()) {
        lFound=true;
        break;
      }
    }
    if (lFound) 
      keepList.push_back(*it);
    else
      delete (*it);
  }
  _client = keepList;

  //  Register filters
  _filters->update(rx);

  //  _update_groups();

  setUpdatesEnabled(true);

  // Let DetectorSelect::discovered know that we are done.
  _discovered = true;
  pthread_cond_signal(&_condition);
}

int DetectorSelect::read_description(Socket& s,int len) {
  _rate_display->read_description(s,len);
  return len;
}

int  DetectorSelect::read_payload    (Socket& s,int len) {
  return _rate_display->read_payload(s,len);
}

bool DetectorSelect::svc             () const { return true; }

void DetectorSelect::process         () {
  _rate_display->process();
}

void DetectorSelect::queue_autosave()
{
  _autosave_timer->stop();
  _autosave_timer->start(10000);
}

static void _save(const QString& fname, char* buffer, int len)
{
  static bool warned = false;
  FILE* o = fopen(qPrintable(fname),"w");
  if (o) {
    fwrite(buffer,len,1,o);
    fclose(o);
    printf("Saved %d bytes to %s\n",len,qPrintable(fname));
  }
  else if (!warned) {
    warned = true;
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    //    QMessageBox::critical(this,"Save Error",msg);
    printf("Save Error: %s\n",qPrintable(msg));
  }
}

void DetectorSelect::autosave()
{

  char* buffer = new char[MaxConfigSize];

  int len = get_setup(buffer);
  _save(QString("%1/AUTOSAVE.ami").arg(Path::base()),buffer,len);
  
  if (Path::archive()) {
    char time_buffer[32];
    time_t seq_tm = time(NULL);
    strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));
    _save(QString("%1/%2.ami").arg(*Path::archive(),
                                   time_buffer),
          buffer,len);
  }

  char* p = buffer;
  XML_insert(p, "Defaults", "Defaults", Defaults::instance()->save(p));
  XML_insert(p, "QtTree", "QtTree", QtTree::save_preferences(p));
  len = p-buffer;
  _save(QString("%1/.defaults.ami").arg(getenv("HOME")),buffer,len);

  delete[] buffer;
}

void DetectorSelect::autoload()
{
  QString fname = QString("%1/AUTOSAVE.ami").arg(Path::base());
  load_setup(qPrintable(fname));
}

void DetectorSelect::run_snapshot()
{
  if (Path::archive()) {
    char time_buffer[32];
    time_t seq_tm = time(NULL);
    strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));
    QString dir = QString("%1/%2/").arg(*Path::archive()).arg(time_buffer);
    mkdir(qPrintable(dir), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP);
    for(std::list<QtTopWidget*>::const_iterator it = _client.begin();
        it != _client.end(); it++)
      if (*it)
        (*it)->snapshot(dir);
  }
}

void DetectorSelect::disconnected()
{
  //  abort();
}
