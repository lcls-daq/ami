#include "ami/qt/JungfrauClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Control.hh"
#include "ami/event/Calib.hh"
#include "ami/event/JungfrauCalib.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

using namespace Ami::Qt;

JungfrauClient::JungfrauClient(QWidget* w,const Pds::DetInfo& i, unsigned u, const QString& n) :
  ImageClient(w, i, u, n),
  _reloadPedestals(false)
{
  addWidget(_npBox = new QCheckBox("Disable\nCorrections"));
  addWidget(_kevBox = new QCheckBox("Corrections in keV"));
  // Set _kevBox state to checked by default
  _kevBox->setChecked(true);

  connect(_npBox, SIGNAL(clicked()), this, SIGNAL(changed()));
  connect(_kevBox, SIGNAL(clicked()), this, SIGNAL(changed()));
}

JungfrauClient::~JungfrauClient() {}

void JungfrauClient::save(char*& p) const
{
  XML_insert(p, "ImageClient", "self", ImageClient::save(p) );
  XML_insert(p, "QCheckBox", "_npBox",  QtPersistent::insert(p,_npBox->isChecked())  );
  XML_insert(p, "QCheckBox", "_kevBox", QtPersistent::insert(p,_kevBox->isChecked()) );
}

void JungfrauClient::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "ImageClient")
      ImageClient::load(p);
    else if (tag.name == "_npBox")
      _npBox->setChecked(QtPersistent::extract_b(p));
    else if (tag.name == "_kevBox")
      _kevBox->setChecked(QtPersistent::extract_b(p));
  XML_iterate_close(JungfrauClient,tag);
}

void JungfrauClient::_configure(char*& p, 
                                unsigned input, 
                                unsigned& output,
                                ChannelDefinition* ch[], 
                                int* signatures, 
                                unsigned nchannels)
{
  unsigned o = 0;
  if (_npBox->isChecked()) {
    o |= JungfrauCalib::option_no_pedestal();
  } else {
    o |= JungfrauCalib::option_correct_gain();
  }
  if (_kevBox->isChecked()) {
    o |= JungfrauCalib::option_pixel_value_in_kev();
  }
  if (_reloadPedestals) {
    o |= JungfrauCalib::option_reload_pedestal();
    _reloadPedestals = false;
  }
  ConfigureRequest& req = *new(p) ConfigureRequest(_input,o);
  p += req.size();

  ImageClient::_configure(p,input,output,ch,signatures,nchannels);
}

void JungfrauClient::retain_corrections(bool v)
{
}
