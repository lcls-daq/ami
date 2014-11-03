#include "L3THandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/psddl/l3t.ddl.h"

#include <stdio.h>

using namespace Ami;

L3THandler::L3THandler(FeatureCache& f)  : 
  EventHandlerF(Pds::ProcInfo(Pds::Level::Event,0,-1U),
		Pds::TypeId::Id_L3TData,
		Pds::TypeId::Id_L3TConfig,
		f)
{
}

L3THandler::~L3THandler()
{
}

void   L3THandler::_calibrate(Pds::TypeId id,
			      const void* payload, 
			      const Pds::ClockTime& t) 
{}

void   L3THandler::_configure(Pds::TypeId id,
			      const void* payload, 
			      const Pds::ClockTime& t) 
{
  _cache.add("DAQ:L3Accept");
  _cache.add("DAQ:L3Bias");
}

void   L3THandler::_event    (Pds::TypeId id,
			      const void* payload, 
			      const Pds::ClockTime& t)
{
  if (_index>=0) {
    switch(id.version()) {
    case 1:
      {
        const Pds::L3T::DataV1& l3t = 
          *reinterpret_cast<const Pds::L3T::DataV1*>(payload);
        _cache.cache(_index+0, l3t.accept()==0 ? 0:1);
        _cache.cache(_index+1, 0);
      } break;
    case 2:
      {
        const Pds::L3T::DataV2& l3t = 
          *reinterpret_cast<const Pds::L3T::DataV2*>(payload);
        _cache.cache(_index+0, l3t.result());
        _cache.cache(_index+1, l3t.bias());
      } break;
    default:
      break;
    }
  }
}

//  No Entry data
unsigned     L3THandler::nentries() const { return 0; }
const Entry* L3THandler::entry   (unsigned) const { return 0; }
void         L3THandler::rename(const char*) {}
