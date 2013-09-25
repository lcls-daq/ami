#include "FEEGasDetEnergyReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>

using namespace Ami;

FEEGasDetEnergyReader::FEEGasDetEnergyReader(FeatureCache& f)  : 
  EventHandlerF(Pds::BldInfo(0,Pds::BldInfo::FEEGasDetEnergy),
		Pds::TypeId::Id_FEEGasDetEnergy,
		Pds::TypeId::Id_FEEGasDetEnergy,
		f),
  _index(-1)
{
}

FEEGasDetEnergyReader::~FEEGasDetEnergyReader()
{
}

void   FEEGasDetEnergyReader::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   FEEGasDetEnergyReader::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t) 
{
  _index = _add_to_cache("BLD:FEE:GDET1:PMT1:ENRC");
  _add_to_cache("BLD:FEE:GDET1:PMT2:ENRC");
  _add_to_cache("BLD:FEE:GDET2:PMT1:ENRC");
  _add_to_cache("BLD:FEE:GDET2:PMT2:ENRC");
}

void   FEEGasDetEnergyReader::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const Pds::Bld::BldDataFEEGasDetEnergy& bld = 
      *reinterpret_cast<const Pds::Bld::BldDataFEEGasDetEnergy*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.f_11_ENRC());
    _cache.cache(index++,bld.f_12_ENRC());
    _cache.cache(index++,bld.f_21_ENRC());
    _cache.cache(index  ,bld.f_22_ENRC());
  }
}

void   FEEGasDetEnergyReader::_damaged  ()
{
  if (_index>=0) {
    unsigned index = _index;
    _cache.cache(index++,-1,true); // f_11
    _cache.cache(index++,-1,true); // f_12
    _cache.cache(index++,-1,true); // f_21
    _cache.cache(index  ,-1,true); // f_22
  }
}

//  No Entry data
unsigned     FEEGasDetEnergyReader::nentries() const { return 0; }
const Entry* FEEGasDetEnergyReader::entry   (unsigned) const { return 0; }
void         FEEGasDetEnergyReader::reset   () 
{
  EventHandlerF::reset();
  _index=-1; 
}
void         FEEGasDetEnergyReader::rename  (const char* s) {}
