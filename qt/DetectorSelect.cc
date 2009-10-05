#include "DetectorSelect.hh"

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/WaveformClient.hh"
#include "ami/qt/ImageClient.hh"
#include "ami/qt/EnvClient.hh"
#include "ami/client/VClientManager.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QFont>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <errno.h>

using namespace Ami::Qt;

enum { Gd=0, Ims=1, Itof=2, Mbes=3, Etof=4, Vmi=9, Bps=10, Env=12, MaxClients=13 };

static const char* names[] = { "GasDet", "IMS", "ITOF", "MBES", 
			       "ETOF_1", "ETOF_2", "ETOF_3", "ETOF_4", "ETOF_5",
			       "VMI", "BPS_0", "BPS_1", "Env" };
static const int MaxConfigSize = 0x100000;

DetectorSelect::DetectorSelect(unsigned interface,
			       unsigned serverGroup) :
  QWidget     (0),
  _interface  (interface),
  _serverGroup(serverGroup),
  _client     (new QtPWidget*[MaxClients]),
  _restore    (0)
{
  for(unsigned k=0; k<MaxClients; k++)
    _client[k] = 0;

  setWindowTitle("AMO Live\nMonitoring");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QVBoxLayout* l = new QVBoxLayout;
  { QLabel* title = new QLabel("AMO Live\nMonitoring");
    QFont font = title->font();
    font.setPointSize(font.pointSize()+8);
    title->setFont(font);
    l->addWidget(title,0,::Qt::AlignHCenter); }
  { QGroupBox* setup_box = new QGroupBox("Setup");
    QVBoxLayout* layout = new QVBoxLayout;
    QPushButton* saveB = new QPushButton("Save");
    QPushButton* loadB = new QPushButton("Load");
    layout->addWidget(saveB);
    layout->addWidget(loadB);
    connect(saveB, SIGNAL(clicked()), this, SLOT(save()));
    connect(loadB, SIGNAL(clicked()), this, SLOT(load()));
    setup_box->setLayout(layout);
    l->addWidget(setup_box); }
  { QGroupBox* data_box  = new QGroupBox("Data");
    QVBoxLayout* layout = new QVBoxLayout;

    QPushButton* imsB  = new QPushButton(names[Ims]);
    QPushButton* itofB = new QPushButton(names[Itof]);
    QPushButton* gdB   = new QPushButton(names[Gd]);
    QPushButton* mbesB = new QPushButton(names[Mbes]);
    QComboBox*   etofB = new QComboBox;
    for(int i=0; i<5; i++)
      etofB->addItem(names[Etof+i]);
    QComboBox*   bpsB  = new QComboBox;
    for(int i=0; i<2; i++)
      bpsB->addItem(names[Bps+i]);
    QPushButton* vmiB  = new QPushButton(names[Vmi]);
    QPushButton* envB  = new QPushButton(names[Env]);
  
    layout->addWidget(gdB  );
    layout->addWidget(imsB );
    layout->addWidget(itofB);
    layout->addWidget(mbesB);
    layout->addWidget(etofB);
    layout->addWidget(bpsB );
    layout->addWidget(vmiB );
    layout->addWidget(envB );

    connect(gdB   , SIGNAL(clicked()), this, SLOT(start_gd()));
    connect(imsB  , SIGNAL(clicked()), this, SLOT(start_ims()));
    connect(itofB , SIGNAL(clicked()), this, SLOT(start_itof()));
    connect(mbesB , SIGNAL(clicked()), this, SLOT(start_mbes()));
    connect(etofB , SIGNAL(activated(int)), this, SLOT(start_etof(int)));
    connect(bpsB  , SIGNAL(activated(int)), this, SLOT(start_bps (int)));
    connect(vmiB  , SIGNAL(clicked()), this, SLOT(start_vmi()));
    connect(envB  , SIGNAL(clicked()), this, SLOT(start_env()));

    data_box->setLayout(layout);
    l->addWidget(data_box); }
  setLayout(l);
}

DetectorSelect::~DetectorSelect()
{
  for(unsigned k=0; k<MaxClients; k++)
    if (_client[k])
      delete _client[k];
}

void DetectorSelect::save       ()
{
  char* buffer = new char[MaxConfigSize];
  char* p = buffer;
  for(unsigned i=0; i<MaxClients; i++)
    if (_client[i]) {
      QtPersistent::insert(p,QString("DetectorSelect"));
      QtPersistent::insert(p,QString(names[i]));
      _client[i]->save(p);
    }
  QtPersistent::insert(p,QString("EndDetectorSelect"));
  
  char time_buffer[32];
  time_t seq_tm = time(NULL);
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def(time_buffer);
  def += ".ami";
  QString fname =     
    QFileDialog::getSaveFileName(this,"Save Setup to File (.ami)",
                                 def,".ami");
  FILE* o = fopen(qPrintable(fname),"w");
  if (o) {
    fwrite(buffer,p-buffer,1,o);
    fclose(o);
  }
  else {
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    QMessageBox::critical(this,"Save Error",msg);
  }

  delete[] buffer;
}

#define LOAD_AND_CONNECT						\
    client->load(p);							\
    VClientManager* manager = new VClientManager(_interface,		\
						 _serverGroup,		\
						 *client);		\
    client->managed(*manager);						\
    _client[i] = client;

#define CASE_WFCLIENT(det) 						\
    Ami::Qt::WaveformClient* client =					\
      new Ami::Qt::WaveformClient(this,					\
				  Pds::DetInfo(0,Pds::DetInfo::det,	\
					       0,Pds::DetInfo::Acqiris,0),0);

void DetectorSelect::load       ()
{
  // get the file 
  QString fname = QFileDialog::getOpenFileName(this,"Load Setup from File (.ami)",
					       ".", "*.ami");

  FILE* f = fopen(qPrintable(fname),"r");
  if (!f) {
    QString msg = QString("Error opening %1 : %2").arg(fname).arg(strerror(errno));
    QMessageBox::critical(this,"Load Error",msg);
    return;
  }
  char* buffer = new char[MaxConfigSize];
  int size = fread(buffer,1,MaxConfigSize,f);
  
  printf("Read %d bytes\n",size);

  const char* p   = buffer;

  //
  //  Create the clients, if necessary, load them, and connect
  //

  // parse the input
  QString name = QtPersistent::extract_s(p);
  while(name==QString("DetectorSelect")) {
    name=QtPersistent::extract_s(p);
    printf("Loading %s\n",qPrintable(name));
    for(unsigned i=0; i<MaxClients; i++)
      if (name==QString(names[i])) {
	if (_client[i])	  
	  _client[i]->load(p);
	else {
	  switch(i) {
	  case Gd:   { CASE_WFCLIENT(AmoGasdet); LOAD_AND_CONNECT } break;
	  case Ims:  { CASE_WFCLIENT(AmoIms   ); LOAD_AND_CONNECT } break;
	  case Itof: { CASE_WFCLIENT(AmoITof  ); LOAD_AND_CONNECT } break;
	  case Mbes: { CASE_WFCLIENT(AmoMbes  ); LOAD_AND_CONNECT } break;
	  case Vmi:  
	    { Ami::Qt::ImageClient* client = 
		new Ami::Qt::ImageClient(this,Pds::DetInfo(0,Pds::DetInfo::AmoVmi,
							   0,Pds::DetInfo::Opal1000,0),0);
	      LOAD_AND_CONNECT }
	    break;
	  case Env:  
	    { Ami::Qt::EnvClient* client = new Ami::Qt::EnvClient(this);
	      LOAD_AND_CONNECT }
	    break;
	  default:
	    if (i<Bps) {
	      Ami::Qt::WaveformClient* client = 
		new Ami::Qt::WaveformClient(this,Pds::DetInfo(0,Pds::DetInfo::AmoETof,
							      0,Pds::DetInfo::Acqiris,0),i-Etof);
	      LOAD_AND_CONNECT }
	    else {
	      Ami::Qt::ImageClient* client = 
		new Ami::Qt::ImageClient(this,Pds::DetInfo(0,Pds::DetInfo::AmoBps,
							   0,Pds::DetInfo::Opal1000,i-Bps),0);
	      LOAD_AND_CONNECT }
	    break;
	  }
	}
      }
    name=QtPersistent::extract_s(p);
  }
  delete[] buffer;
}

void DetectorSelect::start_gd   () { start_waveform_client(Pds::DetInfo::AmoGasdet,0,Gd); }
void DetectorSelect::start_ims  () { start_waveform_client(Pds::DetInfo::AmoIms   ,0,Ims); }
void DetectorSelect::start_itof () { start_waveform_client(Pds::DetInfo::AmoITof  ,0,Itof); }
void DetectorSelect::start_mbes () { start_waveform_client(Pds::DetInfo::AmoMbes  ,0,Mbes); }
void DetectorSelect::start_etof (int channel) { start_waveform_client(Pds::DetInfo::AmoETof,channel,Etof+channel); }
void DetectorSelect::start_vmi  () { start_image_client(Pds::DetInfo::AmoVmi   ,0,Vmi); }
void DetectorSelect::start_env  () { start_features_client(Env); }

void DetectorSelect::start_waveform_client(Pds::DetInfo::Detector det, 
					   unsigned channel,
					   unsigned i)
{
  if (_client[i]) {
    _client[i]->show();
    printf("wf client @ %d,%d\n",_client[i]->pos().x(),_client[i]->pos().y());
  }
  else {
    Pds::DetInfo src(0,det,0,Pds::DetInfo::Acqiris,0);
    Ami::Qt::WaveformClient* client = new Ami::Qt::WaveformClient(this,src,channel);
    VClientManager* manager = new VClientManager(_interface,
						 _serverGroup, 
						 *client);
    client->managed(*manager);
    _client[i] = client;
    printf("wf client @ %d,%d\n",_client[i]->pos().x(),_client[i]->pos().y());
  }
}

void DetectorSelect::start_image_client(Pds::DetInfo::Detector det, 
					unsigned channel,
					unsigned i)
{
  if (_client[i])
    _client[i]->show();
  else {
    Pds::DetInfo src(0,det,0,Pds::DetInfo::Opal1000,0);
    Ami::Qt::ImageClient* client = new Ami::Qt::ImageClient(this,src,channel);
    VClientManager* manager = new VClientManager(_interface,
						 _serverGroup, 
						 *client);
    client->managed(*manager);
    _client[i] = client;
  }
}

void DetectorSelect::start_bps  (int channel) 
{
  unsigned i = Bps+channel;
  if (_client[i])
    _client[i]->show();
  else {
    Pds::DetInfo src(0,Pds::DetInfo::AmoBps,0,Pds::DetInfo::Opal1000,channel);
    Ami::Qt::ImageClient* client = new Ami::Qt::ImageClient(this,src,0);
    VClientManager* manager = new VClientManager(_interface,
						 _serverGroup, 
						 *client);
    client->managed(*manager);
    _client[i] = client;
  }
}

void DetectorSelect::start_features_client(unsigned i)
{
  if (_client[i])
    _client[i]->show();
  else {
    Ami::Qt::EnvClient* client = new Ami::Qt::EnvClient(this);
    VClientManager* manager = new VClientManager(_interface,
						 _serverGroup, 
						 *client);
    client->managed(*manager);
    _client[i] = client;
  }
}

