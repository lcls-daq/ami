#include "IpimbHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/ipimb.ddl.h"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;
using namespace Pds;

IpimbHandler::IpimbHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
		Pds::TypeId::Id_IpimbData,
		Pds::TypeId::Id_IpimbConfig,
		f)
{
}

IpimbHandler::~IpimbHandler()
{
}

void   IpimbHandler::_calibrate(Pds::TypeId id, const void* payload, const Pds::ClockTime& t) {}
void   IpimbHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  char dtitle[64];
  char pstitle[64];

  switch(info().level()) {
  case Level::Reporter:
    strncpy(buffer,Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info())),60);
    strcpy (dtitle ,":DATA[%d]");
    strcpy (pstitle,":PS:DATA[%d]");
    break;
  case Level::Source:
  default:
    strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),60);
    strcpy (dtitle ,"-Ch%d");
    strcpy (pstitle,"-Ch%d:PS");
    break;
  }

  char* iptr = buffer+strlen(buffer);
  for(unsigned i=0; i<NChannels; i++) {
    sprintf(iptr,dtitle,i);
    _add_to_cache(buffer);
  }
  if (id.version()>1) {
    for(unsigned i=0; i<NChannels; i++) {
      sprintf(iptr,pstitle,i);
      _add_to_cache(buffer);
    }
  }
}

void   IpimbHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  int index=_index;
  if (id.version()==1) {
    const Pds::Ipimb::DataV1& d = 
      *reinterpret_cast<const Pds::Ipimb::DataV1*>(payload);
    _cache.cache(index++, d.channel0Volts());
    _cache.cache(index++, d.channel1Volts());
    _cache.cache(index++, d.channel2Volts());
    _cache.cache(index++, d.channel3Volts());
  }
  else if (id.version()==2) {
    const Pds::Ipimb::DataV2& d = 
      *reinterpret_cast<const Pds::Ipimb::DataV2*>(payload);
    _cache.cache(index++, d.channel0Volts());
    _cache.cache(index++, d.channel1Volts());
    _cache.cache(index++, d.channel2Volts());
    _cache.cache(index++, d.channel3Volts());
    _cache.cache(index++, d.channel0psVolts());
    _cache.cache(index++, d.channel1psVolts());
    _cache.cache(index++, d.channel2psVolts());
    _cache.cache(index++, d.channel3psVolts());
  }
  else 
    ;
}

//  No Entry data
unsigned     IpimbHandler::nentries() const { return 0; }
const Entry* IpimbHandler::entry   (unsigned) const { return 0; }
void         IpimbHandler::rename  (const char* s)
{
  if (_index<0) return;

  char buffer[64];
  char dtitle[64];
  char pstitle[64];

  switch(info().level()) {
  case Level::Reporter:
    strcpy (dtitle ,":DATA[%d]");
    strcpy (pstitle,":PS:DATA[%d]");
    break;
  case Level::Source:
  default:
    strcpy (dtitle ,"-Ch%d");
    strcpy (pstitle,"-Ch%d:PS");
    break;
  }

  strncpy(buffer,s,60);
  char* iptr = buffer+strlen(buffer);
  int index=_index;
  for(unsigned i=0; i<NChannels; i++) {
    sprintf(iptr,dtitle,i);
    _rename_cache(index++,buffer);
  }
  for(unsigned i=0; i<NChannels; i++) {
    sprintf(iptr,pstitle,i);
    _rename_cache(index++,buffer);
  }
}
