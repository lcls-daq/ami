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
  EventHandler(Pds::ProcInfo(Pds::Level::Event,0,-1U),
               Pds::TypeId::Id_L3TData,
               Pds::TypeId::Id_L3TConfig),
  _cache(f),
  _index(-1)
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
  _index = _cache.add("DAQ:L3Accept");
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
        _cache.cache(_index, l3t.accept()==0 ? 0:1);
      } break;
    default:
      break;
    }
  }
}

void   L3THandler::_damaged  ()
{
  if (_index>=0)
    _cache.cache(_index,-1,true);
}

//  No Entry data
unsigned     L3THandler::nentries() const { return 0; }
const Entry* L3THandler::entry   (unsigned) const { return 0; }
void         L3THandler::rename(const char*) {}
void         L3THandler::reset() { _index=-1; }
