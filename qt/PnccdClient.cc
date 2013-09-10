#include "ami/qt/PnccdClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Control.hh"
#include "ami/event/PnccdCalib.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

using namespace Ami::Qt;

PnccdClient::PnccdClient(QWidget* w,const Pds::DetInfo& i, unsigned u, const QString& n) :
  ImageClient(w, i, u, n),
  _reloadPedestals(false)
{
  { QPushButton* pedB = new QPushButton("Write Pedestals");
    addWidget(pedB);
    connect(pedB, SIGNAL(clicked()), this, SLOT(write_pedestals())); }

  addWidget(_fnBox = new QCheckBox("Correct\nCommon Mode"));
  addWidget(_npBox = new QCheckBox("Retain\nPedestal"));

  connect(_fnBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_npBox, SIGNAL(clicked()), this, SIGNAL(changed()));
}

PnccdClient::~PnccdClient() {}

void PnccdClient::save(char*& p) const
{
  XML_insert(p, "ImageClient", "self", ImageClient::save(p) );
  XML_insert(p, "QCheckBox", "_fnBox", QtPersistent::insert(p,_fnBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_npBox", QtPersistent::insert(p,_npBox->isChecked()) );
}

void PnccdClient::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "ImageClient")
      ImageClient::load(p);
    else if (tag.name == "_fnBox")
      _fnBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_npBox")
      _npBox->setChecked(QtPersistent::extract_b(p));
  XML_iterate_close(PnccdClient,tag);
}

void PnccdClient::_configure(char*& p, 
                             unsigned input, 
                             unsigned& output,
                             ChannelDefinition* ch[], 
                             int* signatures, 
                             unsigned nchannels)
{
  unsigned o = 0;
  if (_fnBox->isChecked()) o |= PnccdCalib::option_correct_common_mode();
  if (_npBox->isChecked()) o |= PnccdCalib::option_no_pedestal();
  if (_reloadPedestals) {
    o |= PnccdCalib::option_reload_pedestal();
    _reloadPedestals = false;
  }
  ConfigureRequest& req = *new(p) ConfigureRequest(input,o);
  p += req.size();

  ImageClient::_configure(p,input,output,ch,signatures,nchannels);
}

void PnccdClient::write_pedestals()
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

  box.addButton(writeB,QMessageBox::AcceptRole);

  if (box.exec()==QMessageBox::Cancel)
    ;
  else {

    _control->pause();

    std::string smsg(PnccdCalib::save_pedestals(_cds.entry(signature),
                                               !_npBox->isChecked(),
                                               lProd));
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
