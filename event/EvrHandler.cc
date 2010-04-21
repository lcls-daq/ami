#include "EvrHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/evr/ConfigV3.hh"
#include "pdsdata/evr/PulseConfigV3.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>
#include <math.h>

using namespace Ami;

EvrHandler::EvrHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandler(info,
	       Pds::TypeId::Id_EvrData,
	       Pds::TypeId::Id_EvrConfig),
  _cache(f)
{
}

EvrHandler::~EvrHandler()
{
}

void   EvrHandler::_calibrate(const void* payload, const Pds::ClockTime& t) 
{ _configure(payload,t); }

void   EvrHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  const Pds::EvrData::ConfigV3& c = *reinterpret_cast<const Pds::EvrData::ConfigV3*>(payload);

  char buffer[64];

  sprintf(buffer,"DAQ:EVR%d:",static_cast<const Pds::DetInfo&>(info()).devId());
  char* iptr = buffer+strlen(buffer);

  for(unsigned i=0; i<c.npulses(); i++) {
    sprintf(iptr,"P%d:Delay",i);
    int index = _cache.add(buffer);
    double delay = floor(c.pulse(i).delay()*c.pulse(i).prescale())/119.e6;
    _cache.cache(index, delay);
  }
}

void   EvrHandler::_event    (const void* payload, const Pds::ClockTime& t) {}

void   EvrHandler::_damaged  () {}

//  No Entry data
unsigned     EvrHandler::nentries() const { return 0; }
const Entry* EvrHandler::entry   (unsigned) const { return 0; }
void         EvrHandler::reset   () {}
