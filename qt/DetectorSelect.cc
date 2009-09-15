#include "DetectorSelect.hh"

#include "ami/qt/Client.hh"
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
  setWindowTitle("AMO Live\nMonitoring");
  QPushButton* imsB  = new QPushButton("IMS");
  QPushButton* itofB = new QPushButton("ITOF");
  QPushButton* gdB   = new QPushButton("GasDet");
  QPushButton* mbesB = new QPushButton("MBES");
  QComboBox*   etofB = new QComboBox;
  for(int i=0; i<5; i++)
    etofB->addItem(QString("ETOF %1").arg(i));

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
  setLayout(layout);

  connect(gdB   , SIGNAL(clicked()), this, SLOT(start_gd()));
  connect(imsB  , SIGNAL(clicked()), this, SLOT(start_ims()));
  connect(itofB , SIGNAL(clicked()), this, SLOT(start_itof()));
  connect(mbesB , SIGNAL(clicked()), this, SLOT(start_mbes()));
  connect(etofB , SIGNAL(activated(int)), this, SLOT(start_etof(int)));
}

DetectorSelect::~DetectorSelect()
{
}

void DetectorSelect::start_gd   () { start_client(Pds::DetInfo::AmoGasdet,0); }
void DetectorSelect::start_ims  () { start_client(Pds::DetInfo::AmoIms   ,0); }
void DetectorSelect::start_itof () { start_client(Pds::DetInfo::AmoITof  ,0); }
void DetectorSelect::start_mbes () { start_client(Pds::DetInfo::AmoMbes  ,0); }
void DetectorSelect::start_etof (int channel) { start_client(Pds::DetInfo::AmoETof,channel); }

void DetectorSelect::start_client(Pds::DetInfo::Detector det, unsigned channel)
{
  Pds::DetInfo src(0,det,0,Pds::DetInfo::Acqiris,0);
  Ami::Qt::Client* client = new Ami::Qt::Client(src,channel);
  VClientManager* manager = new VClientManager(_interface,
					       _serverGroup, 
					       *client);
  client->managed(*manager);
  manager->connect();
}
