#include "EBeamReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>

using namespace Ami;

EBeamReader::EBeamReader(FeatureCache& f)  : 
  EventHandler(Pds::BldInfo(0,Pds::BldInfo::EBeam),
	       Pds::TypeId::Id_EBeam,
	       Pds::TypeId::Id_EBeam),
  _cache(f),
  _index(-1),
  _nvars(0)
{
}

EBeamReader::~EBeamReader()
{
}

void   EBeamReader::_calibrate(Pds::TypeId id,
                               const void* payload, 
                               const Pds::ClockTime& t) 
{}

void   EBeamReader::_configure(Pds::TypeId id,
                               const void* payload, 
                               const Pds::ClockTime& t) 
{
  _index = _cache.add("BLD:EBEAM:Q");
  _cache.add("BLD:EBEAM:L3E");
  _cache.add("BLD:EBEAM:LTUX");
  _cache.add("BLD:EBEAM:LTUY");
  _cache.add("BLD:EBEAM:LTUXP");
  int index = _cache.add("BLD:EBEAM:LTUYP");

  if (id.version()>=1) {
    index = _cache.add("BLD:EBEAM:PKCURRBC2");
    if (id.version()>=2) {
      index = _cache.add("BLD:EBEAM:PKCURRBC2");
      index = _cache.add("BLD:EBEAM:ENERGYBC2");
      if (id.version()>=3) {
        index = _cache.add("BLD:EBEAM:PKCURRBC1");
        index = _cache.add("BLD:EBEAM:ENERGYBC1");
      }
    }
  }

  _nvars = index-_index+1;
}

void   EBeamReader::_event    (Pds::TypeId id,
                               const void* payload, 
                               const Pds::ClockTime& t)
{
  if (_index>=0) {
    unsigned index = _index;

    switch(id.version()) {
    case 0:
      {
        const Pds::Bld::BldDataEBeamV0& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV0*>(payload);
        _cache.cache(index++,bld.ebeamCharge());
        _cache.cache(index++,bld.ebeamL3Energy());
        _cache.cache(index++,bld.ebeamLTUPosX());
        _cache.cache(index++,bld.ebeamLTUPosY());
        _cache.cache(index++,bld.ebeamLTUAngX());
        _cache.cache(index++,bld.ebeamLTUAngY());
        break;
      }
    case 1:
      {
        const Pds::Bld::BldDataEBeamV1& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV1*>(payload);
        _cache.cache(index++,bld.ebeamCharge());
        _cache.cache(index++,bld.ebeamL3Energy());
        _cache.cache(index++,bld.ebeamLTUPosX());
        _cache.cache(index++,bld.ebeamLTUPosY());
        _cache.cache(index++,bld.ebeamLTUAngX());
        _cache.cache(index++,bld.ebeamLTUAngY());
        _cache.cache(index++,bld.ebeamPkCurrBC2());
        break;
      }
    case 2:
      {
        const Pds::Bld::BldDataEBeamV2& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV2*>(payload);
        _cache.cache(index++,bld.ebeamCharge());
        _cache.cache(index++,bld.ebeamL3Energy());
        _cache.cache(index++,bld.ebeamLTUPosX());
        _cache.cache(index++,bld.ebeamLTUPosY());
        _cache.cache(index++,bld.ebeamLTUAngX());
        _cache.cache(index++,bld.ebeamLTUAngY());
        _cache.cache(index++,bld.ebeamPkCurrBC2());
        _cache.cache(index++,bld.ebeamEnergyBC2());
        break;
      }
    case 3:
    default:
      {
        const Pds::Bld::BldDataEBeamV3& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV3*>(payload);
        _cache.cache(index++,bld.ebeamCharge());
        _cache.cache(index++,bld.ebeamL3Energy());
        _cache.cache(index++,bld.ebeamLTUPosX());
        _cache.cache(index++,bld.ebeamLTUPosY());
        _cache.cache(index++,bld.ebeamLTUAngX());
        _cache.cache(index++,bld.ebeamLTUAngY());
        _cache.cache(index++,bld.ebeamPkCurrBC2());
        _cache.cache(index++,bld.ebeamEnergyBC2());
        _cache.cache(index++,bld.ebeamPkCurrBC1());
        _cache.cache(index++,bld.ebeamEnergyBC1());
        break;
      }
    }
  }
}

void   EBeamReader::_damaged  ()
{
  if (_index>=0) {
    unsigned index = _index;
    for(int i=0; i<_nvars; i++)
      _cache.cache(index++,-1,true);
  }
}

//  No Entry data
unsigned     EBeamReader::nentries() const { return 0; }
const Entry* EBeamReader::entry   (unsigned) const { return 0; }
void         EBeamReader::reset   () { _index=-1; }
void         EBeamReader::rename  (const char*) {}
