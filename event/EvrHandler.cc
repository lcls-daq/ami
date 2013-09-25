#include "EvrHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/evr.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>
#include <math.h>

using namespace Ami;

EvrHandler::EvrHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
		Pds::TypeId::Id_EvrData,
		Pds::TypeId::Id_EvrConfig,
		f)
{
  reset();
}

EvrHandler::~EvrHandler()
{
}

void   EvrHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) 
{ _configure(type,payload,t); }

#define REGISTER_CODE(evtcode) {		\
    unsigned code = evtcode;			\
    sprintf(iptr,"Evt%d",code);			\
    int index = _add_to_cache(buffer);		\
    _cache.cache(index, 0);			\
    _index[code] = index;			\
  }

void   EvrHandler::_configure(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  sprintf(buffer,"DAQ:EVR%d:",static_cast<const Pds::DetInfo&>(info()).devId());
  char* iptr = buffer+strlen(buffer);

  switch(type.version()) {
  case 5:
    { const Pds::EvrData::ConfigV5& c = *reinterpret_cast<const Pds::EvrData::ConfigV5*>(payload);
      sprintf(buffer,"DAQ:EVR:");
      iptr = buffer+strlen(buffer);
      for(unsigned i=0; i<c.neventcodes(); i++) {
        REGISTER_CODE(c.eventcodes()[i].code());
      }
      break; }
  case 6:
    { const Pds::EvrData::ConfigV6& c = *reinterpret_cast<const Pds::EvrData::ConfigV6*>(payload);
      sprintf(buffer,"DAQ:EVR:");
      iptr = buffer+strlen(buffer);
      for(unsigned i=0; i<c.neventcodes(); i++) {
        REGISTER_CODE(c.eventcodes()[i].code());
      }
      break; }
  case 7:
    { const Pds::EvrData::ConfigV7& c = *reinterpret_cast<const Pds::EvrData::ConfigV7*>(payload);
      sprintf(buffer,"DAQ:EVR:");
      iptr = buffer+strlen(buffer);
      for(unsigned i=0; i<c.neventcodes(); i++) {
        REGISTER_CODE(c.eventcodes()[i].code());
      }
      break; }
  default:
    printf("type.version()=%d is not supported\n", type.version());
    return;
  }

  REGISTER_CODE(140);
  REGISTER_CODE(162);
  REGISTER_CODE(163);
}

#undef REGISTER_CODE

void   EvrHandler::_event    (Pds::TypeId type, const void* payload, const Pds::ClockTime& t) 
{
  for(unsigned i=0; i<256; i++)
    if (_index[i]>=0)
      _cache.cache(_index[i],0);

  if (type.version()==3) {
    const Pds::EvrData::DataV3& d = *reinterpret_cast<const Pds::EvrData::DataV3*>(payload);

    for(unsigned i=0; i<d.numFifoEvents(); i++) {
      int index = _index[d.fifoEvents()[i].eventCode()];
      if (index >=0) _cache.cache(index, 1);
    }
  }
}

void   EvrHandler::_damaged  () 
{
  for(unsigned i=0; i<256; i++)
    if (_index[i]>=0)
      _cache.cache(_index[i],0);
}

//  No Entry data
unsigned     EvrHandler::nentries() const { return 0; }
const Entry* EvrHandler::entry   (unsigned) const { return 0; }
void         EvrHandler::reset   () 
{
  EventHandlerF::reset();
  memset(_index, -1, sizeof(_index)); 
}
void         EvrHandler::rename  (const char* s)
{
}
