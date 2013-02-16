#include "ami/qt/FccdClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Control.hh"
#include "ami/event/FccdCalib.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

using namespace Ami::Qt;

FccdClient::FccdClient(QWidget* w,const Pds::DetInfo& i, unsigned u) :
  ImageClient(w, i, u)
{
  { QPushButton* pedB = new QPushButton("Write Pedestals");
    addWidget(pedB);
    connect(pedB, SIGNAL(clicked()), this, SLOT(write_pedestals())); }

  addWidget(_npBox = new QCheckBox("Retain\nPedestal"));

  connect(_npBox, SIGNAL(clicked()), this, SIGNAL(changed()));
}

FccdClient::~FccdClient() {}

void FccdClient::save(char*& p) const
{
  XML_insert(p, "ImageClient", "self", ImageClient::save(p) );
  XML_insert(p, "QCheckBox", "_npBox", QtPersistent::insert(p,_npBox->isChecked()) );
}

void FccdClient::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "ImageClient")
      ImageClient::load(p);
    else if (tag.name == "_npBox")
      _npBox->setChecked(QtPersistent::extract_b(p));
  XML_iterate_close(FccdClient,tag);
}

void FccdClient::_configure(char*& p, 
                             unsigned input, 
                             unsigned& output,
                             ChannelDefinition* ch[], 
                             int* signatures, 
                             unsigned nchannels)
{
  unsigned o = 0;
  if (_npBox->isChecked()) o |= FccdCalib::option_no_pedestal();
  ConfigureRequest& req = *new(p) ConfigureRequest(input,o);
  p += req.size();

  ImageClient::_configure(p,input,output,ch,signatures,nchannels);
}

void FccdClient::write_pedestals()
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

  QPushButton* prodB = new QPushButton("Prod");
  QPushButton* testB = new QPushButton("Test");

  box.addButton(testB,QMessageBox::AcceptRole);

  if (QString(getenv("HOME")).endsWith("opr"))
    box.addButton(prodB,QMessageBox::AcceptRole);

  if (box.exec()==QMessageBox::Cancel)
    ;
  else {

    _control->pause();

    QString msg(FccdCalib::save_pedestals(_cds.entry(signature),
					  box.clickedButton()==prodB).c_str());

    QMessageBox::warning(this, 
                         "Write Pedestals", 
                         msg,
                         QMessageBox::Ok);

    _control->resume();
  }
}
