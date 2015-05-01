#include "AnalogInputHandler.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>

using namespace Ami;

AnalogInputHandler::AnalogInputHandler(const Pds::BldInfo& info, FeatureCache& f)  : 
  EventHandlerF(info,
		Pds::TypeId::Id_AnalogInput,
		Pds::TypeId::Id_AnalogInput,
		f)
{
}

AnalogInputHandler::~AnalogInputHandler()
{
}

void   AnalogInputHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   AnalogInputHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t) 
{
  const char* name = Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info()));
  char* buffer = new char[strlen(name)+8];
  strcpy(buffer,name);
  char* iptr = buffer+strlen(buffer);

  switch(id.version()) {
  case 1:
    { const Pds::Bld::BldDataAnalogInputV1& d = *reinterpret_cast<const Pds::Bld::BldDataAnalogInputV1*>(payload);
      for(unsigned i=0; i<d.numChannels(); i++) {
        sprintf(iptr,":CH%d",i);
        _add_to_cache(buffer);
      }
    } break;
  default:
    break;
  }

  delete[] buffer;
}

void   AnalogInputHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    unsigned index = _index;
    switch(id.version()) {
    case 1:
      { const Pds::Bld::BldDataAnalogInputV1& d = 
          *reinterpret_cast<const Pds::Bld::BldDataAnalogInputV1*>(payload);
        for(unsigned i=0; i<d.numChannels(); i++)
          _cache.cache(index++,d.channelVoltages()[i]);
      } break;
    default:
      break;
    }
  }
}

//  No Entry data
unsigned     AnalogInputHandler::nentries() const { return 0; }
const Entry* AnalogInputHandler::entry   (unsigned) const { return 0; }
void         AnalogInputHandler::rename  (const char* name) 
{
  char* buffer = new char[strlen(name)+8];
  strcpy(buffer,name);
  char* iptr = buffer+strlen(buffer);

  for(unsigned i=0; i<_indices.size(); i++) {
    sprintf(iptr,":CH%d",i);
    _rename_cache(_indices[i],iptr);
  }

  delete[] buffer;
}
