#include "ami/qt/CspadClient.hh"
#include "ami/data/ConfigureRequest.hh"

#include <QtGui/QCheckBox>

using namespace Ami::Qt;

CspadClient::CspadClient(QWidget* w,const Pds::DetInfo& i, unsigned u) :
  ImageClient(w, i, u)
{
  addWidget(_spBox = new QCheckBox("Suppress\nBad Pixels"));
  addWidget(_fnBox = new QCheckBox("Correct\nCommon Mode"));
}

CspadClient::~CspadClient() {}

void CspadClient::save(char*& p) const
{
  ImageClient::save(p);
  QtPersistent::insert(p,_fnBox->isChecked());
  QtPersistent::insert(p,_spBox->isChecked());
}

void CspadClient::load(const char*& p)
{
  ImageClient::load(p);
  _fnBox->setChecked(QtPersistent::extract_b(p));
  _spBox->setChecked(QtPersistent::extract_b(p));
}

void CspadClient::_configure(char*& p, 
                             unsigned input, 
                             unsigned& output,
                             ChannelDefinition* ch[], 
                             int* signatures, 
                             unsigned nchannels)
{
  ImageClient::_configure(p,input,output,ch,signatures,nchannels);
  
  unsigned o = 0;
  if (_fnBox->isChecked()) o |= 2;
  if (_spBox->isChecked()) o |= 1;
  ConfigureRequest& req = *new(p) ConfigureRequest(input,o);
  p += req.size();
}
      
