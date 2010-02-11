#include "DetectorSelect.hh"

#include "ami/qt/QtTopWidget.hh"
#include "ami/qt/WaveformClient.hh"
#include "ami/qt/ImageClient.hh"
#include "ami/qt/EnvClient.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/PrintAction.hh"
#include "ami/qt/DetectorSave.hh"
#include "ami/qt/DetectorReset.hh"
#include "ami/qt/DetectorButton.hh"
#include "ami/qt/DetectorList.hh"
#include "ami/client/VClientManager.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Discovery.hh"

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

#include <errno.h>

using namespace Ami::Qt;

static const int MaxConfigSize = 0x100000;

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
			       unsigned interface,
			       unsigned serverGroup) :
  QtPWidget   (0),
  _interface  (interface),
  _serverGroup(serverGroup),
  _manager    (new VClientManager(interface, serverGroup, *this)),
  _restore    (0),
  _reset_box  (new DetectorReset(this, _client)),
  _save_box   (new DetectorSave (this, _client))
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
    QPushButton* saveB  = new QPushButton("Save");
    QPushButton* loadB  = new QPushButton("Load");
    QPushButton* printB = new QPushButton("Printer");
    layout->addWidget(saveB);
    layout->addWidget(loadB);
    //    layout->addWidget(printB);
    connect(saveB , SIGNAL(clicked()), this, SLOT(save_setup()));
    connect(loadB , SIGNAL(clicked()), this, SLOT(load_setup()));
    connect(printB, SIGNAL(clicked()), this, SLOT(print_setup()));
    setup_box->setLayout(layout);
    l->addWidget(setup_box); }
  { QGroupBox* data_box  = new QGroupBox("Data");
    QVBoxLayout* layout = new QVBoxLayout;    

    QPushButton* resetB = new QPushButton("Reset Plots");
    QPushButton* saveB  = new QPushButton("Save Plots");
    layout->addWidget(resetB);
    layout->addWidget(saveB);
    connect(resetB, SIGNAL(clicked()), this, SLOT(reset_plots()));
    connect(saveB , SIGNAL(clicked()), this, SLOT(save_plots()));

    _client_layout = new QVBoxLayout;
    layout->addLayout(_client_layout);

    QPushButton* envB  = new QPushButton("Env");
    layout->addWidget(envB );
    connect(envB  , SIGNAL(clicked()), this, SLOT(start_env()));

    data_box->setLayout(layout);
    l->addWidget(data_box); }
  setLayout(l);
}

DetectorSelect::~DetectorSelect()
{
  for(std::list<QtTopWidget*>::iterator it = _client.begin();
      it != _client.end(); it++)
    if (*it)
      delete (*it);

  delete _manager;
}

void DetectorSelect::save_setup ()
{
  char* buffer = new char[MaxConfigSize];
  char* p = buffer;

  for(std::list<QtTopWidget*>::iterator it = _client.begin();
      it != _client.end(); it++)
    if ((*it)) {
      QtPersistent::insert(p,QString("DetectorSelect"));
      insertInfo(p, (*it)->info, (*it)->channel);
      QtPersistent::insert(p,QString((*it)->title()));
      (*it)->save(p);
    }
  QtPersistent::insert(p,QString("EndDetectorSelect"));

  save(p);
  _save_box ->save(p);
  _reset_box->save(p);

  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def = QString("%1/%2.ami").arg(Path::base()).arg(time_buffer);
  QString fname =     
    QFileDialog::getSaveFileName(this,"Save Setup to File (.ami)",
                                 def,"*.ami");
  FILE* o = fopen(qPrintable(fname),"w");
  if (o) {
    fwrite(buffer,p-buffer,1,o);
    fclose(o);
    printf("Saved %d bytes to %s\n",p-buffer,qPrintable(fname));
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

  FILE* f = fopen(qPrintable(fname),"r");
  if (!f) {
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    QMessageBox::critical(this,"Load Error",msg);
    return;
  }
  char* buffer = new char[MaxConfigSize];
  int size = fread(buffer,1,MaxConfigSize,f);
  
  printf("Load %d bytes from %s\n",size,qPrintable(fname));

  const char* p   = buffer;

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

    if (!lFound)
      _create_client(info,channel)->load(p);

    name=QtPersistent::extract_s(p);
  }

  load(p);
  _save_box ->load(p);
  _reset_box->load(p);

  delete[] buffer;
}

void DetectorSelect::print_setup()
{
  QPrintDialog* d = new QPrintDialog(PrintAction::printer(),this);
  d->exec();
  delete d;
}

void DetectorSelect::reset_plots()
{
  _reset_box->show();
}

void DetectorSelect::save_plots()
{
  _save_box->show();
}

Ami::Qt::Client* DetectorSelect::_create_client(const Pds::DetInfo& info, 
						unsigned channel)
{
  Ami::Qt::Client* client = 0;
  switch(info.device()) {
  case Pds::DetInfo::Acqiris : client = new Ami::Qt::WaveformClient(this, info, channel); break;
  case Pds::DetInfo::Opal1000: client = new Ami::Qt::ImageClient   (this, info, channel); break;
  default: printf("Device type %x not recognized\n", info.device()); break;
  }
  if (client) {
    VClientManager* manager = new VClientManager(_interface,     
						 _serverGroup,	
						 *client);	
    client->managed(*manager);					
    _client.push_back(client);
    _reset_box->update_list();
    _save_box ->update_list();
  }
  return client;
}
   
void DetectorSelect::start_detector(const Pds::DetInfo& info, 
				    unsigned channel)
{
  for(std::list<QtTopWidget*>::iterator it = _client.begin(); it != _client.end(); it++)
    if ((*it)->info==info && (*it)->channel==channel) {
      (*it)->show();
      return;
    }
  _create_client(info,channel);
}

void DetectorSelect::start_env  ()
{
  const Pds::DetInfo envInfo(0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::Evr,0);
  for(std::list<QtTopWidget*>::iterator it = _client.begin(); it != _client.end(); it++)
    if ((*it)->info==envInfo && (*it)->channel==0) {
      (*it)->show();
      return;
    }

  Ami::Qt::EnvClient* client = new Ami::Qt::EnvClient(this);
  VClientManager* manager = new VClientManager(_interface,
					       _serverGroup, 
					       *client);
  client->managed(*manager);
  _client.push_back(client);
  _reset_box->update_list();
  _save_box ->update_list();
}

 void DetectorSelect::connected       () { _manager->discover(); }

int DetectorSelect:: configure       (iovec* iov) { return 0; }

int DetectorSelect:: configured      () { return 0; }

void DetectorSelect::discovered      (const DiscoveryRx& rx) 
{ 
  setUpdatesEnabled(false);

  //  Remove all buttons
  { 
    QLayoutItem* child;
    while((child=_client_layout->takeAt(0)))
      delete child;
  }

  //  Remove all clients not discovered
  {
    std::list<QtTopWidget*> remove;
    for(std::list<QtTopWidget*>::iterator it = _client.begin(); it != _client.end(); it++) {
      
      bool lFound=false;
      for(const Ami::DescEntry* e = rx.entries(); e < rx.end();
	  e = reinterpret_cast<const Ami::DescEntry*>
	    (reinterpret_cast<const char*>(e) + e->size()))
	if ((*it)->info == e->info() && (*it)->channel == e->channel()) {
	  lFound=true;
	  break;
	}
      if (!lFound)
	remove.push_back(*it);
    }
    for(std::list<QtTopWidget*>::iterator it = remove.begin(); it != remove.end(); it++) {
      _client.remove(*it);
      delete *it;
    }
  }

  //  Register new buttons
  {
    const Ami::DescEntry* n;
    QStringList names;
    for(const Ami::DescEntry* e = rx.entries(); e < rx.end(); e = n) {
      n = reinterpret_cast<const Ami::DescEntry*>
	(reinterpret_cast<const char*>(e) + e->size());
      names.append(e->name());
      if (n < rx.end() && e->info()==n->info()) {
	continue;
      }

      printf("Discovered %s\n",e->name());
      if (e->channel()==0)
	_client_layout->addWidget(new DetectorButton(this, e->name(), e->info()));
      else
	_client_layout->addWidget(new DetectorList  (this, names, e->info(), e->channel()+1));
      names.clear();
    }
  }
  _reset_box->update_list();
  _save_box ->update_list();
  setUpdatesEnabled(true);
}

void DetectorSelect::read_description(Socket& s) {
}

void DetectorSelect::read_payload    (Socket& s) {
}

void DetectorSelect::process         () {
}

