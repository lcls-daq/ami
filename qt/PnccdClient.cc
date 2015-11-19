#include "ami/qt/PnccdClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Control.hh"
#include "ami/qt/Rotator.hh"
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
  _rotator   (new Rotator(*this)),
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
  addWidget(_rotator->widget());
    
  connect(_fnBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_npBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_rotator->box(), SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
}

PnccdClient::~PnccdClient() { delete _rotator; }

void PnccdClient::save(char*& p) const
{
  XML_insert(p, "ImageClient", "self", ImageClient::save(p) );
  XML_insert(p, "QCheckBox", "_fnBox", QtPersistent::insert(p,_fnBox->isChecked()) );
  XML_insert(p, "QCheckBox", "_npBox", QtPersistent::insert(p,_npBox->isChecked()) );
  XML_insert(p, "Rotator"  , "_rotator", _rotator->save(p) );
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
    else if (tag.name == "_rotator")
      _rotator->load(p);
  XML_iterate_close(PnccdClient,tag);
}

void PnccdClient::_prototype(const DescEntry& e)
{
  ImageClient::_prototype(e);
  _rotator->prototype(e);
}

unsigned PnccdClient::_preconfigure(char*&    p,
                                    unsigned  input,
                                    unsigned& output,
                                    ConfigureRequest::Source& source)
{
  return _rotator->configure(p,input,output,source);
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
  //  o |= PnccdCalib::option_rotate(rotation());
  if (_reloadPedestals) {
    o |= PnccdCalib::option_reload_pedestal();
    _reloadPedestals = false;
  }
  ConfigureRequest& req = *new(p) ConfigureRequest(_input,o); // apply options to _input
  p += req.size();

  _calibrator->configure (p,input,output,ch,signatures,nchannels,_input_source);
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

Ami::Rotation PnccdClient::rotation() const { return _rotator->rotation(); }
