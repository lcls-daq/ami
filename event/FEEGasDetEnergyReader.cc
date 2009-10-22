#include "FEEGasDetEnergyReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/bld/bldData.hh"

#include <stdio.h>

using namespace Ami;

FEEGasDetEnergyReader::FEEGasDetEnergyReader(FeatureCache& f)  : 
  EventHandler(Pds::BldInfo(0,Pds::BldInfo::FEEGasDetEnergy),
	       Pds::TypeId::Id_FEEGasDetEnergy,
	       Pds::TypeId::NumberOf),  // expect no configure
  _cache(f),
  _index(-1)
{
}

FEEGasDetEnergyReader::~FEEGasDetEnergyReader()
{
}

//  no configure data will appear from BLD
void   FEEGasDetEnergyReader::_calibrate(const void* payload, const Pds::ClockTime& t) {}
void   FEEGasDetEnergyReader::_configure(const void* payload, const Pds::ClockTime& t) {}

void   FEEGasDetEnergyReader::_event    (const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const Pds::BldDataFEEGasDetEnergy& bld = 
      *reinterpret_cast<const Pds::BldDataFEEGasDetEnergy*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.f_11_ENRC);
    _cache.cache(index++,bld.f_12_ENRC);
    _cache.cache(index++,bld.f_21_ENRC);
    _cache.cache(index  ,bld.f_22_ENRC);
  }
}

void   FEEGasDetEnergyReader::_damaged  ()
{
  unsigned index = _index;
  _cache.cache(index++,-1,true); // f_11
  _cache.cache(index++,-1,true); // f_12
  _cache.cache(index++,-1,true); // f_21
  _cache.cache(index  ,-1,true); // f_22
}

//  No Entry data
unsigned     FEEGasDetEnergyReader::nentries() const { return 0; }
const Entry* FEEGasDetEnergyReader::entry   (unsigned) const { return 0; }
void         FEEGasDetEnergyReader::reset   () 
{
  _index = _cache.add("FEE:GDET:11:ENRC");
  _cache.add("FEE:GDET:12:ENRC");
  _cache.add("FEE:GDET:21:ENRC");
  _cache.add("FEE:GDET:22:ENRC");
}
