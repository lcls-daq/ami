#include "GMDReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/bld/bldData.hh"

#include <stdio.h>

using namespace Ami;

GMDReader::GMDReader(FeatureCache& f)  : 
  EventHandler(Pds::BldInfo(0,Pds::BldInfo::GMD),
         Pds::TypeId::Id_GMD,
         Pds::TypeId::Id_GMD),
  _cache(f),
  _index(-1)
{
}

GMDReader::~GMDReader()
{
}

void   GMDReader::_calibrate(const void* payload, const Pds::ClockTime& t) {}
void   GMDReader::_configure(const void* payload, const Pds::ClockTime& t) 
{
  _index = _cache.add("BLD:GMD:Pressure");
  _cache.add("BLD:GMD:Temperature");
  _cache.add("BLD:GMD:Current");
  _cache.add("BLD:GMD:HvMeshElectron");
  _cache.add("BLD:GMD:HvMeshIon");
  _cache.add("BLD:GMD:HvMultIon");
  _cache.add("BLD:GMD:ChargeQ");
  _cache.add("BLD:GMD:PhotonEnergy");
  _cache.add("BLD:GMD:PhotonsPerPulse");
}

void   GMDReader::_event    (const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const Pds::BldDataGMD& bld = 
      *reinterpret_cast<const Pds::BldDataGMD*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.fPressure);
    _cache.cache(index++,bld.fTemperature);
    _cache.cache(index++,bld.fCurrent);
    _cache.cache(index++,(double)bld.iHvMeshElectron);
    _cache.cache(index++,(double)bld.iHvMeshIon);
    _cache.cache(index++,(double)bld.iHvMultIon);
    _cache.cache(index++,bld.fChargeQ);
    _cache.cache(index++,bld.fPhotonEnergy);
    _cache.cache(index++,bld.fMultPulseIntensity);
    _cache.cache(index++,bld.fKeithleyPulseIntensity);
    _cache.cache(index++,bld.fPulseEnergy);
    _cache.cache(index++,bld.fPulseEnergyFEE);
    _cache.cache(index++,bld.fTransmission);
    _cache.cache(index++,bld.fTransmissionFEE);        
  }
}

void   GMDReader::_damaged  ()
{
  if (_index>=0) {
    unsigned index = _index;
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index++,-1,true);
    _cache.cache(index  ,-1,true);
  }
}

//  No Entry data
unsigned     GMDReader::nentries() const { return 0; }
const Entry* GMDReader::entry   (unsigned) const { return 0; }
void         GMDReader::reset   () { _index=-1; }
