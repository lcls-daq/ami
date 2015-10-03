#include "ami/qt/PnccdClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Control.hh"
#include "ami/qt/PnccdCalibrator.hh"
#include "ami/event/PnccdCalib.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>

using namespace Ami::Qt;

PnccdClient::PnccdClient(QWidget* w,const Pds::DetInfo& i, unsigned u, const QString& n) :
  ImageClient(w, i, u, n),
  _reloadPedestals(false)
{
  { QPushButton* calB = new QPushButton("Calibrate");
    addWidget(calB);
    _calibrator = new PnccdCalibrator(this);
    _stack->add(calB,_calibrator);
    addWidget(calB);
    connect(_calibrator, SIGNAL(changed()), this, SIGNAL(changed())); }
  
  addWidget(_fnBox = new QCheckBox("Correct\nCommon Mode"));
  addWidget(_npBox = new QCheckBox("Retain\nPedestal"));

  { QChar degree(0x00B0);
    _roBox = new QComboBox;
    _roBox->addItem(QString("  0"));
    _roBox->addItem(QString(" 90%1").arg(degree));
    _roBox->addItem(QString("180%1").arg(degree));
    _roBox->addItem(QString("270%1").arg(degree));
    QWidget* w = new QWidget;
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(_roBox);
    hl->addWidget(new QLabel("Rotate\nDisplay"));
    w->setLayout(hl);
    addWidget(w); }
    
  connect(_fnBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_npBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_roBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
}

PnccdClient::~PnccdClient() {}

void PnccdClient::save(char*& p) const
{
  XML_insert(p, "ImageClient", "self", ImageClient::save(p) );
  XML_insert(p, "QCheckBox", "_fnBox", QtPersistent::insert(p,_fnBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_npBox", QtPersistent::insert(p,_npBox->isChecked()) );
  XML_insert(p, "QComboBox", "_roBox", QtPersistent::insert(p,_roBox->currentIndex()) );
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
    else if (tag.name == "_roBox")
      _roBox->setCurrentIndex(QtPersistent::extract_i(p));
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
  o |= PnccdCalib::option_rotate(rotation());
  if (_reloadPedestals) {
    o |= PnccdCalib::option_reload_pedestal();
    _reloadPedestals = false;
  }
  ConfigureRequest& req = *new(p) ConfigureRequest(input,o);
  p += req.size();

  _calibrator->configure (p,input,output,ch,signatures,nchannels);
  ImageClient::_configure(p,input,output,ch,signatures,nchannels);
}

void PnccdClient::_setup_payload(Cds& cds)
{
  _calibrator->setup_payload(cds);
  ImageClient::_setup_payload(cds);
}

void PnccdClient::_update()
{
  _calibrator->update();
  ImageClient::_update();
}

Ami::Rotation PnccdClient::rotation() const { return Rotation(_roBox->currentIndex()); }
