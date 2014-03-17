#include "BldSpectrometerHandler.hh"

#include "ami/data/EntryWaveform.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <stdio.h>

using namespace Ami;

BldSpectrometerHandler::BldSpectrometerHandler(const Pds::BldInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_Spectrometer, Pds::TypeId::Id_Spectrometer),
  _nentries(0)
{
}

BldSpectrometerHandler::~BldSpectrometerHandler()
{
}

unsigned BldSpectrometerHandler::nentries() const { return _nentries; }

const Entry* BldSpectrometerHandler::entry(unsigned i) const 
{
  if (i<_nentries)
    return _entry[i];
  else
    return 0;
}

void BldSpectrometerHandler::rename(const char* s)
{
  char buff[64];

  sprintf(buff,"%s_H",s);
  _entry[0]->desc().name(buff);
  sprintf(buff,"%s_V",s);
  _entry[1]->desc().name(buff);
}

void BldSpectrometerHandler::reset() {
  _nentries=0;
}

void BldSpectrometerHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void BldSpectrometerHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const char* name = BldInfo::name(static_cast<const BldInfo&>(info()));
  const DetInfo& dInfo = static_cast<const DetInfo&>(info());
  char buff[64];
  { sprintf(buff,"%s_H",name);
    DescWaveform desc(dInfo, 0, buff,
                      "Pixel","Sum", 1024, 0., 1024.);
    _entry[0] = new EntryWaveform(desc); }
  { sprintf(buff,"%s_V",name);
    DescWaveform desc(dInfo, 1, buff,
                      "Pixel","Sum", 256, 0., 256.);
    _entry[1] = new EntryWaveform(desc); }
  _nentries=2;
}

void BldSpectrometerHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::Bld::BldDataSpectrometerV0& d = *reinterpret_cast<const Pds::Bld::BldDataSpectrometerV0*>(payload);

  const uint32_t* h = d.hproj().data();
  for(unsigned i=0; i<1024; i++)
    _entry[0]->content(double(h[i]),i);
  _entry[0]->info(1,EntryWaveform::Normalization);
  _entry[0]->valid(t);

  const uint32_t* v = d.vproj().data();
  for(unsigned i=0; i<256; i++)
    _entry[1]->content(double(v[i]),i);
  _entry[1]->info(1,EntryWaveform::Normalization);
  _entry[1]->valid(t);
}

void BldSpectrometerHandler::_damaged() 
{
  if (_nentries) {
    if (_entry[0]) _entry[0]->invalid();
    if (_entry[1]) _entry[1]->invalid();
  }
}
