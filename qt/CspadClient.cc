#include "ami/qt/CspadClient.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

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
  unsigned o = 0;
  if (_fnBox->isChecked()) o |= 2;
  if (_spBox->isChecked()) o |= 1;
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
        if (_fnBox->isChecked()) oo |= 2;
        if (_spBox->isChecked()) oo |= 1;
        if (o != oo) {
          printf("CspadClient::setup_payload options %x -> %x\n",oo,o);
        }

        _fnBox->setChecked(o&2);
        _spBox->setChecked(o&1);
        break;
      }
    }
  }
#endif
  ImageClient::_setup_payload(cds);
}      
