#include "ami/qt/CspadClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Control.hh"
#include "ami/event/CspadCalib.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

using namespace Ami::Qt;

CspadClient::CspadClient(QWidget* w,const Pds::DetInfo& i, unsigned u) :
  ImageClient(w, i, u),
  _reloadPedestals(false)
{
  { QPushButton* pedB = new QPushButton("Write Pedestals");
    addWidget(pedB);
    connect(pedB, SIGNAL(clicked()), this, SLOT(write_pedestals())); }

  addWidget(_spBox = new QCheckBox("Suppress\nBad Pixels"));
  addWidget(_fnBox = new QCheckBox("Correct\nCommon Mode"));
  addWidget(_npBox = new QCheckBox("Retain\nPedestal"));
  addWidget(_piBox = new QCheckBox("Post\nIntegral"));

  connect(_spBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_fnBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_npBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_piBox, SIGNAL(clicked()), this, SIGNAL(changed()));
}

CspadClient::~CspadClient() {}

void CspadClient::save(char*& p) const
{
  XML_insert(p, "ImageClient", "self", ImageClient::save(p) );
  XML_insert(p, "QCheckBox", "_fnBox", QtPersistent::insert(p,_fnBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_spBox", QtPersistent::insert(p,_spBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_npBox", QtPersistent::insert(p,_npBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_piBox", QtPersistent::insert(p,_piBox->isChecked()) );
}

void CspadClient::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "ImageClient")
      ImageClient::load(p);
    else if (tag.name == "_fnBox")
      _fnBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_spBox")
      _spBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_npBox")
      _npBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_piBox")
      _piBox->setChecked(QtPersistent::extract_b(p));
  XML_iterate_close(CspadClient,tag);
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
  if (_spBox->isChecked()) o |= CspadCalib::option_suppress_bad_pixels();
  if (_npBox->isChecked()) o |= CspadCalib::option_no_pedestal();
  if (_piBox->isChecked()) o |= CspadCalib::option_post_integral();
  if (_reloadPedestals) {
    o |= CspadCalib::option_reload_pedestal();
    _reloadPedestals=false;
  }
  ConfigureRequest& req = *new(p) ConfigureRequest(input,o);
  p += req.size();

  ImageClient::_configure(p,input,output,ch,signatures,nchannels);
}

void CspadClient::_setup_payload(Cds& cds)
{
#if 0
  if (_input_entry) {
    //  Multiple display clients compete for these options
    for(unsigned i=0; i<NCHANNELS; i++) {
      unsigned signature = _channels[i]->signature();
      const Entry* e = cds.entry(signature);
      if (e) {
        unsigned o = e->desc().options();

        unsigned oo(0);
        if (_fnBox->isChecked()) oo |= CspadCalib::option_correct_common_mode();
        if (_spBox->isChecked()) oo |= CspadCalib::option_suppress_bad_pixels();
        if (_npBox->isChecked()) oo |= CspadCalib::option_no_pedestal();
        if (_piBox->isChecked()) oo |= CspadCalib::option_post_integral();
        if (o != oo) {
          printf("CspadClient::setup_payload options %x -> %x\n",oo,o);
        }

        _fnBox->setChecked(o&CspadCalib::option_correct_common_mode());
        _spBox->setChecked(o&CspadCalib::option_suppress_bad_pixels());
        break;
      }
    }
  }
#endif
  ImageClient::_setup_payload(cds);
}      

void CspadClient::write_pedestals()
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

    std::string smsg(CspadCalib::save_pedestals(_cds.entry(signature),!_npBox->isChecked(),lProd));
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
