#include "DetectorSelect.hh"

#include "ami/qt/QtTopWidget.hh"
#include "ami/qt/WaveformClient.hh"
#include "ami/qt/ImageClient.hh"
#include "ami/qt/EnvClient.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/PrintAction.hh"
#include "ami/qt/DetectorSave.hh"
#include "ami/qt/DetectorReset.hh"
#include "ami/client/VClientManager.hh"

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

#ifndef CAMP
enum { Gd=0, Ims=2, Itof=3, Mbes=4, Etof=6, Vmi=11, Bps=12, Env=14, MaxClients=15 };

static const char* names[] = { "GasDet_1", "GasDet_2", "IMS", "ITOF", "MBES_1", "MBES_2", 
			       "ETOF_1", "ETOF_2", "ETOF_3", "ETOF_4", "ETOF_5",
			       "VMI", "BPS_0", "BPS_1", "Env" };
#else
enum { Evr=0, CampAcq=2, CampVmi=22, Env=23, MaxClients=24 };

static const char* names[] = { "EVR_1", "EVR_2", 
			       "ACQ_1" , "ACQ_2" , "ACQ_3" , "ACQ_4" , "ACQ_5" ,
			       "ACQ_6" , "ACQ_7" , "ACQ_8" , "ACQ_9" , "ACQ_10",
			       "ACQ_11", "ACQ_12", "ACQ_13", "ACQ_14", "ACQ_15",
			       "ACQ_16", "ACQ_17", "ACQ_18", "ACQ_19", "ACQ_20",
			       "VMI", "Env" };
#endif

static const int MaxConfigSize = 0x100000;

DetectorSelect::DetectorSelect(const QString& label,
			       unsigned interface,
			       unsigned serverGroup) :
  QtPWidget   (0),
  _interface  (interface),
  _serverGroup(serverGroup),
  _client     (new QtTopWidget*[MaxClients]),
  _restore    (0),
  _reset_box  (new DetectorReset(this, _client, names, MaxClients)),
  _save_box   (new DetectorSave (this, _client, names, MaxClients))
{
  for(unsigned k=0; k<MaxClients; k++)
    _client[k] = 0;

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

#ifndef CAMP
    QPushButton* imsB  = new QPushButton(names[Ims]);
    QPushButton* itofB = new QPushButton(names[Itof]);
    QComboBox*   gdB  = new QComboBox;
    for(int i=0; i<2; i++)
      gdB->addItem(names[Gd+i]);
    QComboBox*   mbesB  = new QComboBox;
    for(int i=0; i<2; i++)
      mbesB->addItem(names[Mbes+i]);
    QComboBox*   etofB = new QComboBox;
    for(int i=0; i<5; i++)
      etofB->addItem(names[Etof+i]);
    QComboBox*   bpsB  = new QComboBox;
    for(int i=0; i<2; i++)
      bpsB->addItem(names[Bps+i]);
    QPushButton* vmiB  = new QPushButton(names[Vmi]);

    layout->addWidget(gdB  );
    layout->addWidget(imsB );
    layout->addWidget(itofB);
    layout->addWidget(mbesB);
    layout->addWidget(etofB);
    layout->addWidget(bpsB );
    layout->addWidget(vmiB );

    connect(gdB   , SIGNAL(activated(int)), this, SLOT(start_gd(int)));
    connect(imsB  , SIGNAL(clicked()), this, SLOT(start_ims()));
    connect(itofB , SIGNAL(clicked()), this, SLOT(start_itof()));
    connect(mbesB , SIGNAL(activated(int)), this, SLOT(start_mbes(int)));
    connect(etofB , SIGNAL(activated(int)), this, SLOT(start_etof(int)));
    connect(bpsB  , SIGNAL(activated(int)), this, SLOT(start_bps (int)));
    connect(vmiB  , SIGNAL(clicked()), this, SLOT(start_vmi()));
#else
    QComboBox*   evrB  = new QComboBox;
    for(int i=0; i<2; i++)
      evrB->addItem(names[Evr+i]);
    QComboBox*   acqB = new QComboBox;
    for(int i=0; i<20; i++)
      acqB->addItem(names[CampAcq+i]);
    QPushButton* vmiB  = new QPushButton(names[CampVmi]);
    layout->addWidget(evrB);
    layout->addWidget(acqB);
    layout->addWidget(vmiB );
    connect(evrB , SIGNAL(activated(int)), this, SLOT(start_evrmon (int)));
    connect(acqB , SIGNAL(activated(int)), this, SLOT(start_campacq(int)));
    connect(vmiB , SIGNAL(clicked())     , this, SLOT(start_campvmi()));
#endif

    QPushButton* envB  = new QPushButton(names[Env]);
    layout->addWidget(envB );
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

void DetectorSelect::save_setup ()
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
  while(name==QString("DetectorSelect")) {
    name=QtPersistent::extract_s(p);
    printf("Loading %s\n",qPrintable(name));
    for(unsigned i=0; i<MaxClients; i++)
      if (name==QString(names[i])) {
	if (_client[i])	  
	  _client[i]->load(p);
	else {
	  switch(i) {
#ifndef CAMP
	  case Ims:  { CASE_WFCLIENT(AmoIms   ); LOAD_AND_CONNECT } break;
	  case Itof: { CASE_WFCLIENT(AmoITof  ); LOAD_AND_CONNECT } break;
	  case Vmi:  
	    { Ami::Qt::ImageClient* client = 
		new Ami::Qt::ImageClient(this,Pds::DetInfo(0,Pds::DetInfo::AmoVmi,
							   0,Pds::DetInfo::Opal1000,0),0);
	      LOAD_AND_CONNECT }
	    break;
	  case Mbes+0:
	  case Mbes+1:
	    { Ami::Qt::WaveformClient* client = 
		new Ami::Qt::WaveformClient(this,Pds::DetInfo(0,Pds::DetInfo::AmoMbes,
							      0,Pds::DetInfo::Acqiris,0),i-Mbes);
	      LOAD_AND_CONNECT }
	    break;
	  case Gd+0:
	  case Gd+1:
	    { Ami::Qt::WaveformClient* client = 
		new Ami::Qt::WaveformClient(this,Pds::DetInfo(0,Pds::DetInfo::AmoGasdet,
							      0,Pds::DetInfo::Acqiris,0),i-Gd);
	      LOAD_AND_CONNECT }
	    break;
	  case Etof+0:
	  case Etof+1:
	  case Etof+2:
	  case Etof+3:
	  case Etof+4:
	    { Ami::Qt::WaveformClient* client = 
		new Ami::Qt::WaveformClient(this,Pds::DetInfo(0,Pds::DetInfo::AmoETof,
							      0,Pds::DetInfo::Acqiris,0),i-Etof);
	      LOAD_AND_CONNECT }
	    break;
	  case Bps+0:
	  case Bps+1:
	    { Ami::Qt::ImageClient* client = 
		new Ami::Qt::ImageClient(this,Pds::DetInfo(0,Pds::DetInfo::AmoBps,
							   0,Pds::DetInfo::Opal1000,i-Bps),0);
	      LOAD_AND_CONNECT }
	    break;
#else
	  case Evr+0:
	  case Evr+1:
	    { Ami::Qt::WaveformClient* client = 
		new Ami::Qt::WaveformClient(this,Pds::DetInfo(0,Pds::DetInfo::NoDetector,
							      0,Pds::DetInfo::Acqiris,0),i-Evr);
	      LOAD_AND_CONNECT }
	    break;
	  case CampAcq+0:
	  case CampAcq+1:
	  case CampAcq+2:
	  case CampAcq+3:
	  case CampAcq+4:
	  case CampAcq+5:
	  case CampAcq+6:
	  case CampAcq+7:
	  case CampAcq+8:
	  case CampAcq+9:
	  case CampAcq+10:
	  case CampAcq+11:
	  case CampAcq+12:
	  case CampAcq+13:
	  case CampAcq+14:
	  case CampAcq+15:
	  case CampAcq+16:
	  case CampAcq+17:
	  case CampAcq+18:
	  case CampAcq+19:
	    { Ami::Qt::WaveformClient* client = 
		new Ami::Qt::WaveformClient(this,Pds::DetInfo(0,Pds::DetInfo::Camp,
							      0,Pds::DetInfo::Acqiris,0),i-CampAcq);
	      LOAD_AND_CONNECT }
	    break;
	  case CampVmi:  
	    { Ami::Qt::ImageClient* client = 
		new Ami::Qt::ImageClient(this,Pds::DetInfo(0,Pds::DetInfo::Camp,
							   0,Pds::DetInfo::Opal1000,0),0);
	      LOAD_AND_CONNECT }
	    break;
#endif
	  case Env:  
	    { Ami::Qt::EnvClient* client = new Ami::Qt::EnvClient(this);
	      LOAD_AND_CONNECT }
	    break;
	  default:
	    break;
	  }
	  _save_box ->enable(i);
	  _reset_box->enable(i);
	}
      }
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

#ifndef CAMP
void DetectorSelect::start_gd   (int channel) { start_waveform_client(Pds::DetInfo::AmoGasdet,channel,Gd+channel); }
void DetectorSelect::start_ims  () { start_waveform_client(Pds::DetInfo::AmoIms   ,0,Ims); }
void DetectorSelect::start_itof () { start_waveform_client(Pds::DetInfo::AmoITof  ,0,Itof); }
void DetectorSelect::start_mbes (int channel) { start_waveform_client(Pds::DetInfo::AmoMbes,channel,Mbes+channel); }
void DetectorSelect::start_etof (int channel) { start_waveform_client(Pds::DetInfo::AmoETof,channel,Etof+channel); }
void DetectorSelect::start_vmi  () { start_image_client(Pds::DetInfo::AmoVmi   ,0,Vmi); }
#else
void DetectorSelect::start_evrmon (int channel) { start_waveform_client(Pds::DetInfo::AmoMbes,channel,Evr+channel); }
void DetectorSelect::start_campacq(int channel) { start_waveform_client(Pds::DetInfo::Camp,channel,CampAcq+channel); }
void DetectorSelect::start_campvmi  () { start_image_client(Pds::DetInfo::Camp   ,0,CampVmi); }
#endif

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
    _save_box ->enable(i);
    _reset_box->enable(i);
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
    _save_box ->enable(i);
    _reset_box->enable(i);
  }
}

#ifndef CAMP
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
    _save_box ->enable(i);
    _reset_box->enable(i);
  }
}
#endif

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
    _save_box ->enable(i);
    _reset_box->enable(i);
  }
}

