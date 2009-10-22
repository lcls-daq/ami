#include "PhaseCavityReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/bld/bldData.hh"

#include <stdio.h>

using namespace Ami;

PhaseCavityReader::PhaseCavityReader(FeatureCache& f)  : 
  EventHandler(Pds::BldInfo(0,Pds::BldInfo::PhaseCavity),
	       Pds::TypeId::Id_PhaseCavity,
	       Pds::TypeId::NumberOf),  // expect no configure
  _cache(f),
  _index(-1)
{
}

PhaseCavityReader::~PhaseCavityReader()
{
}

//  no configure data will appear from BLD
void   PhaseCavityReader::_calibrate(const void* payload, const Pds::ClockTime& t) {}
void   PhaseCavityReader::_configure(const void* payload, const Pds::ClockTime& t) {}

void   PhaseCavityReader::_event    (const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const Pds::BldDataPhaseCavity& bld = 
      *reinterpret_cast<const Pds::BldDataPhaseCavity*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.fFitTime1);
    _cache.cache(index++,bld.fFitTime2);
    _cache.cache(index++,bld.fCharge1);
    _cache.cache(index  ,bld.fCharge2);
  }
}

void   PhaseCavityReader::_damaged  ()
{
  unsigned index = _index;
  _cache.cache(index++,-1,true);
  _cache.cache(index++,-1,true);
  _cache.cache(index++,-1,true);
  _cache.cache(index  ,-1,true);
}

//  No Entry data
unsigned     PhaseCavityReader::nentries() const { return 0; }
const Entry* PhaseCavityReader::entry   (unsigned) const { return 0; }
void         PhaseCavityReader::reset   () 
{
  _index = _cache.add("PHASECAV:T1");
  _cache.add("PHASECAV:T2");
  _cache.add("PHASECAV:Q1");
  _cache.add("PHASECAV:Q2");
}
