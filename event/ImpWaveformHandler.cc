#include "ImpWaveformHandler.hh"

#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <stdio.h>

using namespace Ami;

static Pds::Imp::ConfigV1 _default( 0x1111, 0x5555, 0x98968200, 0x1999,
				    0x2fb5, 0x3fff, 0x927c, 0x3ff, 0x100, 0x1f4);

ImpWaveformHandler::ImpWaveformHandler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_ImpData, Pds::TypeId::Id_ImpConfig),
  _config(_default),
  _nentries(0),
  _ref(NULL)
{
}

ImpWaveformHandler::ImpWaveformHandler(const Pds::DetInfo& info,
				       const Pds::Imp::ConfigV1& config) :
  EventHandler(info, Pds::TypeId::Id_ImpData, Pds::TypeId::Id_ImpConfig),
  _config(_default),
  _nentries(0)
{
  Pds::ClockTime t;
  _configure(Pds::TypeId(Pds::TypeId::Id_ImpConfig,1), &config, t);
}

ImpWaveformHandler::~ImpWaveformHandler()
{
}

//unsigned ImpWaveformHandler::nentries() const { return _nentries + (_ref != NULL); }
unsigned ImpWaveformHandler::nentries() const { return (_ref != NULL); }

const Entry* ImpWaveformHandler::entry(unsigned i) const 
{
//  if (i<_nentries)
//    return _entry[i];
//  else
    return _ref; 
}

void ImpWaveformHandler::rename(const char* s)
{
  if (_ref) _ref->desc().name(s);
}

void ImpWaveformHandler::reset() {
  _nentries = 0;
  _ref = NULL;
}

void ImpWaveformHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void ImpWaveformHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  _nentries = NumberOfEntries;
  const Pds::Imp::ConfigV1& c = *reinterpret_cast<const Pds::Imp::ConfigV1*>(payload);
  _numberOfSamples = _config.numberOfSamples();
  char s[128] = {""};
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  for(unsigned k=0; k<NumberOfEntries; k++) {
    sprintf(s, "IMP Chan%u", k);
    DescWaveform desc(det, k, s, "Samples","ADU", _numberOfSamples, 0., _numberOfSamples);
    _entry[k] = new EntryWaveform(desc);
  }

  if (NumberOfEntries > 1) {
    Channel channelMask((1<<NumberOfEntries)-1, Channel::BitMask);
    _ref = new EntryRef(DescRef(det, channelMask,
                                ChannelID::name(det,channelMask)));
    _ref->set(_entry);
  }

  _config = c;
}

typedef Pds::Imp::ElementV1 ImpD;

void ImpWaveformHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  ImpD* d = const_cast<ImpD*>(reinterpret_cast<const ImpD*>(payload));

  for (unsigned i=0;i<_nentries;i++) {
    EntryWaveform* entry = _entry[i];
    for (unsigned j=0;j<_numberOfSamples;j++) {
      double val = d->samples(_config)[j].channels()[i];
      entry->content(val,j);
    }
    entry->info(1,EntryWaveform::Normalization);
    entry->valid(t);
  }

  if (_ref)
    _ref->valid(t);
}

void ImpWaveformHandler::_damaged() 
{
  for (unsigned i=0;i<_nentries;i++) {
    EntryWaveform* entry = _entry[i];
    if (entry) entry->invalid();
  }

  if (_ref)
    _ref->invalid();
}
