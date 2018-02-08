#include "Generic1DHandler.hh"

#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <stdio.h>

typedef Pds::Generic1D::ConfigV0 G1DCfg;   

using namespace Ami;

  uint32_t _Channels = 16;
  uint32_t _Length[16]= {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
  uint32_t _SampleType[16]= {G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16, G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32};
  int32_t _Offset[16]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double _Period[16]= {8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7};

static Pds::Generic1D::ConfigV0 _default(_Channels, 0,0,0,0);

Generic1DHandler::Generic1DHandler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_Generic1DData, Pds::TypeId::Id_Generic1DConfig),
  _cbuffer(new char[_default._sizeof()]),
  _config(new(_cbuffer) G1DCfg(_Channels, _Length, _SampleType, _Offset, _Period)),
  _nentries(0)
  //_ref(NULL)
{

}

Generic1DHandler::~Generic1DHandler()
{
delete[] _cbuffer;
}

unsigned Generic1DHandler::nentries() const { return (_ref.size()); }

const Entry* Generic1DHandler::entry(unsigned i) const 
{
//  if (i<_nentries)
//    return _entry[i];
//  else
    return _ref[i]; 
}

void Generic1DHandler::rename(const char* s)
{
char buff[64];
for(unsigned i=0; i<_ref.size(); i++) {
    sprintf(buff,"%s%s",s,strchr(_ref[i]->desc().name(), '_'));
    _ref[i]->desc().name(buff);
  }
}

void Generic1DHandler::reset() {
 _ref.clear();
  _nentries = 0;
}

void Generic1DHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void Generic1DHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
//printf ("Check configure\n");
  const Pds::Generic1D::ConfigV0& c = *reinterpret_cast<const Pds::Generic1D::ConfigV0*>(payload);
  delete[] _cbuffer;
  _cbuffer = new char[c._sizeof()];
  _config= new (_cbuffer) G1DCfg(c);

  _nentries = c.NChannels();
  _entry.resize(_nentries);
  _entry2.resize(_nentries);
  char s[128] = {""};
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());

  for(unsigned k=0; k<_nentries; k++) {
    double doff = double(c.Offset()[k]);
    double dlen = double(c.Length()[k]);
    double dtau = double(c.Period()[k]);
    float xlo = doff*dtau;
    float xhi = (dlen+doff)*dtau;
    DescWaveform desc(det, k, s, "Samples","ADU", _config->Length()[k], xlo, xhi);
    _entry[k] = new EntryWaveform(desc);
  }


  if (_nentries > 1) {

	std::vector<double> freqlist;
	freqlist.push_back(_config->Period()[0]);

	for(unsigned i=0; i<_nentries; i++){
        bool match= false;
		for(unsigned y=0; y<freqlist.size(); y++){
			if (freqlist[y] == _config->Period()[i]) {
			match = true;
			break;
			}
		}
	if(!match){
        freqlist.push_back(_config->Period()[i]);
	}
	}

	int n2=0;
	for(unsigned k=0; k<freqlist.size(); k++) {
		unsigned mask=0;
		int n20=n2;
        	for(unsigned j=0; j<_nentries; j++) {
			if(freqlist[k] == _config->Period()[j]){
			mask |= 1<<j;	
			_entry2[n2++]=_entry[j];
			}						
		}
	Channel channelMask(mask, Channel::BitMask);
        char name[100]; 
        double f=1/freqlist[k];
        double logf= log10(f);
        int base= int (logf/3);
 	static const char* cbase[] = {"Hz", "KHz", "MHz", "GHz"};
		if(base<0 || base>3){
	        sprintf(name, "generic1d_%d", k);
		}
	else {
        sprintf(name, "generic1d_%d_%s", int(f/(pow(10,3*base))), cbase[base]);
	}
        EntryRef* E = new EntryRef(DescRef(det, channelMask, name));
        E->set(&_entry2[n20]);
	_ref.push_back(E);
	}
 
}
  

}


typedef Pds::Generic1D::DataV0 G1DData;

void Generic1DHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  G1DData* d = const_cast<G1DData*>(reinterpret_cast<const G1DData*>(payload));

#define FILL(ctype, gtype) {\
ndarray<ctype, 1> a= d->data_##gtype(*_config, z);\
for(unsigned k=0; k<a.shape()[0]; k++) {\
if (k == 0);\
_entry[z]->content(a[k],k);}\
}


for(unsigned z=0; z<_nentries; z++) {
switch (_config->SampleType()[z]) {
    case G1DCfg::UINT8: FILL(const uint8_t, u8); break; 
    case G1DCfg::UINT16: FILL(const uint16_t, u16); break;
    case G1DCfg::UINT32: FILL(const uint32_t, u32); break;
    case G1DCfg::FLOAT32: FILL(const float, f32); break;
    case G1DCfg::FLOAT64: FILL(const double, f64); break;
    } 
_entry[z]->info(1,EntryWaveform::Normalization);
_entry[z]->valid(t);
}



for (unsigned i=0; i<_ref.size(); i++){
    _ref[i]->valid(t);
	}
}

void Generic1DHandler::_damaged() 
{
  for (unsigned i=0;i<_nentries;i++) {
    EntryWaveform* entry = _entry[i];
    if (entry) entry->invalid();
  }

for (unsigned i=0; i<_ref.size(); i++){
    _ref[i]->invalid();
	}
}
