#include "ami/qt/EpixClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Control.hh"
#include "ami/event/FrameCalib.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

using namespace Ami::Qt;

EpixClient::EpixClient(QWidget* w,const Pds::DetInfo& i, unsigned u, const QString& n) :
  ImageClient(w, i, u, n),
  _reloadPedestals(false)
{
  { QPushButton* pedB = new QPushButton("Write Pedestals");
    addWidget(pedB);
    connect(pedB, SIGNAL(clicked()), this, SLOT(write_pedestals())); }

  addWidget(_npBox = new QCheckBox("Retain\nPedestal"));
  addWidget(_fnBox = new QCheckBox("Correct\nCommon Mode"));
  connect(_npBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_fnBox, SIGNAL(clicked()), this, SIGNAL(changed()));
}

EpixClient::~EpixClient() {}

void EpixClient::save(char*& p) const
{
  XML_insert(p, "ImageClient", "self", ImageClient::save(p) );
  XML_insert(p, "QCheckBox", "_npBox", QtPersistent::insert(p,_npBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_fnBox", QtPersistent::insert(p,_fnBox->isChecked()) );
}

void EpixClient::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "ImageClient")
      ImageClient::load(p);
    else if (tag.name == "_npBox")
      _npBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_fnBox")
      _fnBox->setChecked(QtPersistent::extract_b(p));
  XML_iterate_close(EpixClient,tag);
}

void EpixClient::_configure(char*& p, 
			    unsigned input, 
			    unsigned& output,
			    ChannelDefinition* ch[], 
			    int* signatures, 
			    unsigned nchannels)
{
  unsigned o = 0;
  if (_npBox->isChecked()) o |= FrameCalib::option_no_pedestal();
  if (_fnBox->isChecked()) o |= FrameCalib::option_correct_common_mode();
  if (_reloadPedestals) {
    o |= FrameCalib::option_reload_pedestal();
    _reloadPedestals=false;
  }
  ConfigureRequest& req = *new(p) ConfigureRequest(input,o);
  p += req.size();

  ImageClient::_configure(p,input,output,ch,signatures,nchannels);
}

void EpixClient::_setup_payload(Cds& cds)
{
  ImageClient::_setup_payload(cds);
}      

void EpixClient::write_pedestals()
{
  QString name;
  unsigned signature=-1U;

  for(int i=0; i<NCHANNELS; i++)
    if (_channels[i]->is_shown()) {
      name = _channels[i]->name();
      signature = _channels[i]->output_signature();
    }

  QString msg = QString("Write a new pedestal file \nwith the offsets in %1?").arg(name);

  QMessageBox box;
  box.setWindowTitle("Write Pedestals");
  box.setText(msg);
  box.addButton(QMessageBox::Cancel);

  QPushButton* writeB = new QPushButton("Write");

  bool lProd = QString(getenv("HOME")).endsWith("opr");
  if (!lProd) {
    int so = socket(AF_INET, SOCK_DGRAM, 0);
    if (so) {
      ifreq ifr;
      strcpy(ifr.ifr_name,"eth0");
      int rv = ioctl(so, SIOCGIFADDR, (char*)&ifr);
      ::close(so);
      if (rv==0) {
	unsigned interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
	if (((interface>>8)&0xff)==10)
	  lProd=true;
      }
    }
  }

  box.addButton(writeB,QMessageBox::AcceptRole);

  if (box.exec()==QMessageBox::Cancel)
    ;
  else {

    _control->pause();

    std::string smsg(FrameCalib::save_pedestals(_cds.entry(signature),!_npBox->isChecked(),lProd));
    if (smsg.empty()) {
      _reloadPedestals = true;
      emit changed();
    }
    else {
      QString msg(smsg.c_str());
      QMessageBox::warning(this, 
                           "Write Pedestals", 
                           msg,
                           QMessageBox::Ok);
    }

    _control->resume();
  }
}
