#include "OceanOpticsHandler.hh"

#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <stdio.h>

using namespace Ami;

static Pds::OceanOptics::ConfigV1 _default(0.001);

OceanOpticsHandler::OceanOpticsHandler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_OceanOpticsData, Pds::TypeId::Id_OceanOpticsConfig),
  _config(_default),
  _nentries(0)
{
}

OceanOpticsHandler::OceanOpticsHandler(const Pds::DetInfo&   info, 
				       const Pds::OceanOptics::ConfigV1& config) :
  EventHandler(info, Pds::TypeId::Id_OceanOpticsData, Pds::TypeId::Id_OceanOpticsConfig),
  _config(_default),
  _nentries(0)
{
  Pds::ClockTime t;
  _configure(Pds::TypeId(Pds::TypeId::Id_OceanOpticsConfig,1), &config, t);
}

OceanOpticsHandler::~OceanOpticsHandler()
{
}

unsigned OceanOpticsHandler::nentries() const { return _nentries; }

const Entry* OceanOpticsHandler::entry(unsigned i) const 
{
  if (i<_nentries)
    return _entry[i];
  else
    return NULL;
}

void OceanOpticsHandler::rename(const char* s)
{
  if (_entry[0]) _entry[0]->desc().name(s);
}

void OceanOpticsHandler::reset() {
  _nentries = 0;
}

void OceanOpticsHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void OceanOpticsHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::OceanOptics::ConfigV1& c = *reinterpret_cast<const Pds::OceanOptics::ConfigV1*>(payload);

  ndarray<const double,1> p = c.waveLenCalib();
  unsigned  channelNumber = 0;  
  double    fMinX         = p[0];
  double    fMaxX         = p[0] + p[1]*double(Pds::OceanOptics::DataV1::iNumPixels - 1);
  
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescWaveform desc(det, channelNumber,ChannelID::name(det,channelNumber),
    "Wavelength [m]","Count", 3840, fMinX, fMaxX);
    
  _entry[_nentries++] = new EntryWaveform(desc);
  _config = c;
}

void OceanOpticsHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  Pds::OceanOptics::DataV1* d = (Pds::OceanOptics::DataV1*)payload;
  
  EntryWaveform* entry  = _entry[0];
  
  for (int j=0;j<Pds::OceanOptics::DataV1::iNumPixels;j++)
    entry->content( d->nonlinerCorrected(_config, j),j);
    
  entry->info(1,EntryWaveform::Normalization);
  entry->valid(t);
}

void OceanOpticsHandler::_damaged() 
{
  for (unsigned i=0;i<_nentries;i++) {
    EntryWaveform* entry = _entry[i];
    if (entry) entry->invalid();
  }
}
