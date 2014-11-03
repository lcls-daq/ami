#include "EvrHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/evr.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>
#include <math.h>

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_EvrConfig);
  types.push_back(Pds::TypeId::Id_EvsConfig);
  return types;
}

EvrHandler::EvrHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
		Pds::TypeId::Id_EvrData,
		config_type_list(),
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
    _indexcode[code] = index;			\
  }

void   EvrHandler::_configure(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  sprintf(buffer,"DAQ:EVR%d:",static_cast<const Pds::DetInfo&>(info()).devId());
  char* iptr = buffer+strlen(buffer);

  switch(type.id()) {
  case Pds::TypeId::Id_EvrConfig:
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
    break;
  case Pds::TypeId::Id_EvsConfig:
    switch(type.version()) {
    case 1:
      { const Pds::EvrData::SrcConfigV1& c = *reinterpret_cast<const Pds::EvrData::SrcConfigV1*>(payload);
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
    break;
  default:
    break;
  }
}

#undef REGISTER_CODE

void   EvrHandler::_event    (Pds::TypeId type, const void* payload, const Pds::ClockTime& t) 
{
  for(unsigned i=0; i<256; i++)
    if (_indexcode[i]>=0)
      _cache.cache(_indexcode[i],0);

  if (type.version()==3) {
    const Pds::EvrData::DataV3& d = *reinterpret_cast<const Pds::EvrData::DataV3*>(payload);

    for(unsigned i=0; i<d.numFifoEvents(); i++) {
      int index = _indexcode[d.fifoEvents()[i].eventCode()];
      if (index >=0) _cache.cache(index, 1);
    }
  }
}

//  No Entry data
unsigned     EvrHandler::nentries() const { return 0; }
const Entry* EvrHandler::entry   (unsigned) const { return 0; }
void         EvrHandler::reset   () 
{
  EventHandlerF::reset();
  memset(_indexcode, -1, sizeof(_indexcode)); 
}
void         EvrHandler::rename  (const char* s)
{
}
