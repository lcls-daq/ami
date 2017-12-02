#include "AcqWaveformHandler.hh"

#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <stdio.h>

using namespace Ami;
using namespace Pds;

static Pds::Acqiris::VertV1 _default_vert[Pds::Acqiris::ConfigV1::MaxChan];

static Pds::Acqiris::ConfigV1 _default(0,0,0,
				       Pds::Acqiris::TrigV1(0,0,0,0),
				       Pds::Acqiris::HorizV1(0,0,0,0),
				       _default_vert);

AcqWaveformHandler::AcqWaveformHandler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_AcqWaveform, Pds::TypeId::Id_AcqConfig),
  _config(_default),
  _nentries(0),
  _ref(NULL)
{
}

AcqWaveformHandler::AcqWaveformHandler(const Pds::DetInfo&   info, 
				       const Pds::Acqiris::ConfigV1& config) :
  EventHandler(info, Pds::TypeId::Id_AcqWaveform, Pds::TypeId::Id_AcqConfig),
  _config(_default),
  _nentries(0)
{
  Pds::ClockTime t;
  _configure(Pds::TypeId(Pds::TypeId::Id_AcqConfig,1), &config, t);
}

AcqWaveformHandler::~AcqWaveformHandler()
{
}

unsigned AcqWaveformHandler::nentries() const { return _nentries + (_ref != NULL); }

const Entry* AcqWaveformHandler::entry(unsigned i) const 
{
  if (i<_nentries)
    return _entry[i];
  else
    return _ref; 
}

void AcqWaveformHandler::rename(const char* s)
{
  char buff[64];
  
  for(unsigned i=0; i<_nentries; i++) {
    sprintf(buff,"%s_%d",s,i+1);
    _entry[i]->desc().name(buff);
  }
  if (_ref)
    _ref->desc().name(s);
}

void AcqWaveformHandler::reset() {
  _nentries = 0;
  _ref = NULL;
}

void AcqWaveformHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void AcqWaveformHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::Acqiris::ConfigV1& c = *reinterpret_cast<const Pds::Acqiris::ConfigV1*>(payload);
  const Pds::Acqiris::HorizV1& h = c.horiz();
  unsigned channelMask = c.channelMask();
  unsigned channelNumber = 0;
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  for(unsigned k=0; channelMask!=0; k++) {
    if (channelMask&1) {
      DescWaveform desc(det, channelNumber,
			ChannelID::name(det,channelNumber),
			"Time [s]","Voltage [V]",
			h.nbrSamples(), 0., h.sampInterval()*h.nbrSamples());
      _entry[_nentries++] = new EntryWaveform(desc);
      channelNumber++;
    }
    channelMask >>= 1;
  }

  channelMask = c.channelMask();
  if (channelMask & (channelMask-1)) {
    Channel ch(channelMask,Channel::BitMask);
    _ref = new EntryRef(DescRef(det,ch,ChannelID::name(det,ch)));
    _ref->set(_entry);
  }

  _config = c;
}

typedef Pds::Acqiris::DataDescV1 AcqDD;

void AcqWaveformHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  AcqDD* d = const_cast<AcqDD*>(reinterpret_cast<const AcqDD*>(payload));

  unsigned n = _nentries < _config.nbrChannels() ? _nentries : _config.nbrChannels();
  ndarray<const Acqiris::VertV1,1> vert = _config.vert();
  for (unsigned i=0;i<n;i++) {
    ndarray<const int16_t,2> wfs = d->data(_config,i).waveforms(_config);
    double slope  = vert[i].slope();
    double offset = vert[i].offset();
    EntryWaveform* entry = _entry[i];
    for (unsigned j=0;j<wfs.shape()[1];j++) {
      int16_t data = wfs(0,j);
      //        data = (data&0xff<<8) | (data&0xff00>>8);
      double val = double(data)*slope-offset;
      entry->content(val,j);
    }
    entry->info(1,EntryWaveform::Normalization);
    entry->valid(t);
  }

  if (_ref)
    _ref->valid(t);
}

void AcqWaveformHandler::_damaged() 
{
  unsigned n = _nentries < _config.nbrChannels() ? _nentries : _config.nbrChannels();
  for (unsigned i=0;i<n;i++) {
    EntryWaveform* entry = _entry[i];
    if (entry)
      entry->invalid();
  }

  if (_ref)
    _ref->invalid();
}
