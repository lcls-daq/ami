#include "DetectorSelect.hh"

#include "ami/qt/WaveformClient.hh"
#include "ami/qt/ImageClient.hh"
#include "ami/qt/EnvClient.hh"
#include "ami/client/VClientManager.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QFont>

using namespace Ami::Qt;


DetectorSelect::DetectorSelect(unsigned interface,
			       unsigned serverGroup) :
  QWidget     (0),
  _interface  (interface),
  _serverGroup(serverGroup)
{
  for(unsigned k=0; k<MaxClients; k++)
    _client[k] = 0;

  setWindowTitle("AMO Live\nMonitoring");
  setAttribute(::Qt::WA_DeleteOnClose, false);

  QPushButton* imsB  = new QPushButton("IMS");
  QPushButton* itofB = new QPushButton("ITOF");
  QPushButton* gdB   = new QPushButton("GasDet");
  QPushButton* mbesB = new QPushButton("MBES");
  QComboBox*   etofB = new QComboBox;
  for(int i=0; i<5; i++)
    etofB->addItem(QString("ETOF %1").arg(i));
  QComboBox*   bpsB  = new QComboBox;
  for(int i=0; i<2; i++)
    bpsB->addItem(QString("BPS %1").arg(i));
  QPushButton* vmiB  = new QPushButton("VMI");
  QPushButton* envB  = new QPushButton("Env");

  QLabel* title = new QLabel("AMO Live\nMonitoring");
  QFont font = title->font();
  font.setPointSize(font.pointSize()+8);
  title->setFont(font);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(title,0,::Qt::AlignHCenter);
  layout->addWidget(gdB  );
  layout->addWidget(imsB );
  layout->addWidget(itofB);
  layout->addWidget(mbesB);
  layout->addWidget(etofB);
  layout->addWidget(bpsB );
  layout->addWidget(vmiB );
  layout->addWidget(envB );
  setLayout(layout);

  connect(gdB   , SIGNAL(clicked()), this, SLOT(start_gd()));
  connect(imsB  , SIGNAL(clicked()), this, SLOT(start_ims()));
  connect(itofB , SIGNAL(clicked()), this, SLOT(start_itof()));
  connect(mbesB , SIGNAL(clicked()), this, SLOT(start_mbes()));
  connect(etofB , SIGNAL(activated(int)), this, SLOT(start_etof(int)));
  connect(bpsB  , SIGNAL(activated(int)), this, SLOT(start_bps (int)));
  connect(vmiB  , SIGNAL(clicked()), this, SLOT(start_vmi()));
  connect(envB  , SIGNAL(clicked()), this, SLOT(start_env()));
}

DetectorSelect::~DetectorSelect()
{
  for(unsigned k=0; k<MaxClients; k++)
    if (_client[k])
      delete _client[k];
}

void DetectorSelect::start_gd   () { start_waveform_client(Pds::DetInfo::AmoGasdet,0,0); }
void DetectorSelect::start_ims  () { start_waveform_client(Pds::DetInfo::AmoIms   ,0,1); }
void DetectorSelect::start_itof () { start_waveform_client(Pds::DetInfo::AmoITof  ,0,2); }
void DetectorSelect::start_mbes () { start_waveform_client(Pds::DetInfo::AmoMbes  ,0,3); }
void DetectorSelect::start_etof (int channel) { start_waveform_client(Pds::DetInfo::AmoETof,channel,4+channel); }
void DetectorSelect::start_vmi  () { start_image_client(Pds::DetInfo::AmoVmi   ,0,9); }
void DetectorSelect::start_env  () { start_features_client(12); }

void DetectorSelect::start_waveform_client(Pds::DetInfo::Detector det, 
					   unsigned channel,
					   unsigned i)
{
  if (_client[i])
    _client[i]->show();
  else {
    Pds::DetInfo src(0,det,0,Pds::DetInfo::Acqiris,0);
    Ami::Qt::WaveformClient* client = new Ami::Qt::WaveformClient(this,src,channel);
    VClientManager* manager = new VClientManager(_interface,
						 _serverGroup, 
						 *client);
    client->managed(*manager);
    _client[i] = client;
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
  unsigned i = 10+channel;
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

