#include "PhaseCavityReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>

using namespace Ami;

PhaseCavityReader::PhaseCavityReader(FeatureCache& f)  : 
  EventHandler(Pds::BldInfo(0,Pds::BldInfo::PhaseCavity),
	       Pds::TypeId::Id_PhaseCavity,
	       Pds::TypeId::Id_PhaseCavity),
  _cache(f),
  _index(-1)
{
}

PhaseCavityReader::~PhaseCavityReader()
{
}

void   PhaseCavityReader::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   PhaseCavityReader::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t) 
{
  _index = _cache.add("BLD:PHASECAV:T1");
  _cache.add("BLD:PHASECAV:T2");
  _cache.add("BLD:PHASECAV:Q1");
  _cache.add("BLD:PHASECAV:Q2");
}

void   PhaseCavityReader::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const Pds::Bld::BldDataPhaseCavity& bld = 
      *reinterpret_cast<const Pds::Bld::BldDataPhaseCavity*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.fitTime1());
    _cache.cache(index++,bld.fitTime2());
    _cache.cache(index++,bld.charge1());
    _cache.cache(index  ,bld.charge2());
  }
}

void   PhaseCavityReader::_damaged  ()
{
  if (_index>=0) {
    unsigned index = _index;
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index  ,-1,true);
  }
}

//  No Entry data
unsigned     PhaseCavityReader::nentries() const { return 0; }
const Entry* PhaseCavityReader::entry   (unsigned) const { return 0; }
void         PhaseCavityReader::reset   () { _index=-1; }
