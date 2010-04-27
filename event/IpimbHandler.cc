#include "IpimbHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/ipimb/DataV1.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

IpimbHandler::IpimbHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandler(info,
	       Pds::TypeId::Id_IpimbData,
	       Pds::TypeId::Id_IpimbConfig),
  _cache(f)
{
}

IpimbHandler::~IpimbHandler()
{
}

void   IpimbHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}
void   IpimbHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  const Pds::Ipimb::ConfigV1& c = *reinterpret_cast<const Pds::Ipimb::ConfigV1*>(payload);
  _config = c;

  char buffer[64];

  strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),60);
  char* iptr = buffer+strlen(buffer);
  
  for(unsigned i=0; i<NChannels; i++) {
    sprintf(iptr,"[%d]",i);
    _index[i] = _cache.add(buffer);
  }
}

void   IpimbHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  const Pds::Ipimb::DataV1& d = *reinterpret_cast<const Pds::Ipimb::DataV1*>(payload);

  _cache.cache(_index[0], d.channel0());
  _cache.cache(_index[1], d.channel1());
  _cache.cache(_index[2], d.channel2());
  _cache.cache(_index[3], d.channel3());
}

void   IpimbHandler::_damaged  ()
{
}

//  No Entry data
unsigned     IpimbHandler::nentries() const { return 0; }
const Entry* IpimbHandler::entry   (unsigned) const { return 0; }
void         IpimbHandler::reset   () 
{
}