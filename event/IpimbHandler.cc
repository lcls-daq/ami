#include "IpimbHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/ipimb.ddl.h"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

IpimbHandler::IpimbHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandler(info,
	       Pds::TypeId::Id_IpimbData,
	       Pds::TypeId::Id_IpimbConfig),
  _cache (f)
{
}

IpimbHandler::~IpimbHandler()
{
}

void   IpimbHandler::_calibrate(Pds::TypeId id, const void* payload, const Pds::ClockTime& t) {}
void   IpimbHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  char* iptr;

  switch(info().level()) {
  case Level::Reporter:
    strncpy(buffer,Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info())),60);
    iptr = buffer+strlen(buffer);
    for(unsigned i=0; i<NChannels; i++) {
      sprintf(iptr,":DATA[%d]",i);
      _index[i] = _cache.add(buffer);
    }
    if (id.version()>1) {
      for(unsigned i=0; i<NChannels; i++) {
        sprintf(iptr,":PS:DATA[%d]",i);
        _index[i+NChannels] = _cache.add(buffer);
      }
    }
    else
      for(unsigned i=0; i<NChannels; i++)
        _index[i+NChannels] = _index[0];

    break;
  case Level::Source:
  default:
    strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),60);
    iptr = buffer+strlen(buffer);
    for(unsigned i=0; i<NChannels; i++) {
      sprintf(iptr,"-Ch%d",i);
      _index[i] = _cache.add(buffer);
    }
    if (id.version()>1) {
      for(unsigned i=0; i<NChannels; i++) {
        sprintf(iptr,"-Ch%d:PS",i);
        _index[i+NChannels] = _cache.add(buffer);
      }
    }
    else
      for(unsigned i=0; i<NChannels; i++)
        _index[i+NChannels] = _index[0];

    break;
  }
}

void   IpimbHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  if (id.version()==1) {
    const Pds::Ipimb::DataV1& d = 
      *reinterpret_cast<const Pds::Ipimb::DataV1*>(payload);
    _cache.cache(_index[0], d.channel0Volts());
    _cache.cache(_index[1], d.channel1Volts());
    _cache.cache(_index[2], d.channel2Volts());
    _cache.cache(_index[3], d.channel3Volts());
  }
  else if (id.version()==2) {
    const Pds::Ipimb::DataV2& d = 
      *reinterpret_cast<const Pds::Ipimb::DataV2*>(payload);
    _cache.cache(_index[0], d.channel0Volts());
    _cache.cache(_index[1], d.channel1Volts());
    _cache.cache(_index[2], d.channel2Volts());
    _cache.cache(_index[3], d.channel3Volts());
    _cache.cache(_index[4], d.channel0psVolts());
    _cache.cache(_index[5], d.channel1psVolts());
    _cache.cache(_index[6], d.channel2psVolts());
    _cache.cache(_index[7], d.channel3psVolts());
  }
  else 
    ;
}

void   IpimbHandler::_damaged  ()
{
  for(unsigned i=0; i<2*NChannels; i++)
    _cache.cache(_index[i], 0, true);
}

//  No Entry data
unsigned     IpimbHandler::nentries() const { return 0; }
const Entry* IpimbHandler::entry   (unsigned) const { return 0; }
void         IpimbHandler::reset   () 
{
}
