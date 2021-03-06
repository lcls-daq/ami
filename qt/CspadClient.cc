#include "ami/qt/CspadClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Control.hh"
#include "ami/qt/Rotator.hh"
#include "ami/event/CspadCalib.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"
#include "ami/service/Ins.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

using namespace Ami::Qt;

CspadClient::CspadClient(QWidget* w,const Pds::DetInfo& i, unsigned u, const QString& n) :
  ImageClient(w, i, u, n),
  _rotator   (new Rotator(*this)),
  _reloadPedestals(false)
{
  if (Ami::CspadCalib::show_write_pedestals())
  { QPushButton* pedB = new QPushButton("Write Pedestals");
    addWidget(pedB);
    connect(pedB, SIGNAL(clicked()), this, SLOT(write_pedestals())); }

  addWidget(_spBox = new QCheckBox("Suppress\nBad Pixels"));
  addWidget(_fnBox = new QCheckBox("Correct\nCommon Mode"));
  addWidget(_unBox = new QCheckBox("Correct\nUnbonded"));
  addWidget(_npBox = new QCheckBox("Retain Pedestal"));
  addWidget(_gnBox = new QCheckBox("Correct Gain"));
  addWidget(_piBox = new QCheckBox("Post Integral"));
  addWidget(_rotator->widget());

  connect(_spBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_fnBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_unBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_npBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_gnBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_piBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_rotator->box(), SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
}

CspadClient::~CspadClient() { delete _rotator; }

void CspadClient::save(char*& p) const
{
  XML_insert(p, "ImageClient", "self", ImageClient::save(p) );
  XML_insert(p, "QCheckBox", "_fnBox", QtPersistent::insert(p,_fnBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_unBox", QtPersistent::insert(p,_unBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_spBox", QtPersistent::insert(p,_spBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_npBox", QtPersistent::insert(p,_npBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_gnBox", QtPersistent::insert(p,_gnBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_piBox", QtPersistent::insert(p,_piBox->isChecked()) );
  XML_insert(p, "Rotator"  , "_rotator", _rotator->save(p) );
}

void CspadClient::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "ImageClient")
      ImageClient::load(p);
    else if (tag.name == "_fnBox")
      _fnBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_unBox")
      _unBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_spBox")
      _spBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_npBox")
      _npBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_gnBox")
      _gnBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_piBox")
      _piBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_rotator")
      _rotator->load(p);
  XML_iterate_close(CspadClient,tag);
}

void CspadClient::_prototype(const DescEntry& e)
{
  ImageClient::_prototype(e);
  _rotator->prototype(e);
}

unsigned CspadClient::_preconfigure(char*&    p,
                                    unsigned  input,
                                    unsigned& output,
                                    ConfigureRequest::Source& source)
{
  return _rotator->configure(p,input,output,source);
}

void CspadClient::_configure(char*& p, 
                             unsigned input, 
                             unsigned& output,
                             ChannelDefinition* ch[], 
                             int* signatures, 
                             unsigned nchannels)
{
  unsigned o = 0;
  if (_fnBox->isChecked()) o |= CspadCalib::option_correct_common_mode();
  if (_unBox->isChecked()) o |= CspadCalib::option_correct_unbonded();
  if (_spBox->isChecked()) o |= CspadCalib::option_suppress_bad_pixels();
  if (_npBox->isChecked()) o |= CspadCalib::option_no_pedestal();
  if (_gnBox->isChecked()) o |= CspadCalib::option_correct_gain();
  if (_piBox->isChecked()) o |= CspadCalib::option_post_integral();
  if (_reloadPedestals) {
    o |= CspadCalib::option_reload_pedestal();
    _reloadPedestals=false;
  }
  ConfigureRequest& req = *new(p) ConfigureRequest(_input,o);
  p += req.size();

  ImageClient::_configure(p,input,output,ch,signatures,nchannels);
}

void CspadClient::_setup_payload(Cds& cds)
{
  ImageClient::_setup_payload(cds);
}      

void CspadClient::write_pedestals()
{
  QString name;
  unsigned signature=-1U;

  for(unsigned i=0; i<NCHANNELS; i++)
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
  bool lReqFull = true;
  if (!lProd) {
    unsigned interface = Ami::Ins::parse_interface("eth0");
    if (((interface>>8)&0xff)==10) {
      lProd=true;
      lReqFull=false;
    }
  }

  box.addButton(writeB,QMessageBox::AcceptRole);

  if (box.exec()==QMessageBox::Cancel)
    ;
  else {

    _control->pause();

    std::string smsg;
    if (!_npBox->isChecked())
      smsg = std::string("Retain Pedestal must be checked.\n");
    else
      smsg = CspadCalib::save_pedestals(_cds.entry(signature),lProd,lReqFull);

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

Ami::Rotation CspadClient::rotation() const { return _rotator->rotation(); }
