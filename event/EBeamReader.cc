#include "EBeamReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/bld/bldData.hh"

#include <stdio.h>

using namespace Ami;

EBeamReader::EBeamReader(FeatureCache& f)  : 
  EventHandler(Pds::BldInfo(0,Pds::BldInfo::EBeam),
	       Pds::TypeId::Id_EBeam,
	       Pds::TypeId::NumberOf),  // expect no configure
  _cache(f),
  _index(-1)
{
}

EBeamReader::~EBeamReader()
{
}

//  no configure data will appear from BLD
void   EBeamReader::_calibrate(const void* payload, const Pds::ClockTime& t) {}
void   EBeamReader::_configure(const void* payload, const Pds::ClockTime& t) {}

void   EBeamReader::_event    (const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const Pds::BldDataEBeam& bld = 
      *reinterpret_cast<const Pds::BldDataEBeam*>(payload);
    if (bld.uDamageMask)
      _damaged();
    else {
      unsigned index = _index;
      _cache.cache(index++,bld.fEbeamCharge);
      _cache.cache(index++,bld.fEbeamL3Energy);
      _cache.cache(index++,bld.fEbeamLTUPosX);
      _cache.cache(index++,bld.fEbeamLTUPosY);
      _cache.cache(index++,bld.fEbeamLTUAngX);
      _cache.cache(index++,bld.fEbeamLTUAngY);
    }
  }
}

void   EBeamReader::_damaged  ()
{
  unsigned index = _index;
  _cache.cache(index++,-1,true);
  _cache.cache(index++,-1,true);
  _cache.cache(index++,-1,true);
  _cache.cache(index++,-1,true);
  _cache.cache(index++,-1,true);
  _cache.cache(index  ,-1,true);
}

//  No Entry data
unsigned     EBeamReader::nentries() const { return 0; }
const Entry* EBeamReader::entry   (unsigned) const { return 0; }
void         EBeamReader::reset   () 
{
  _index = _cache.add("EBEAM:Q");
  _cache.add("EBEAM:L3E");
  _cache.add("EBEAM:LTUX");
  _cache.add("EBEAM:LTUY");
  _cache.add("EBEAM:LTUXP");
  _cache.add("EBEAM:LTUYP");
}
