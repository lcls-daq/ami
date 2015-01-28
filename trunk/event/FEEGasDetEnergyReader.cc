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
		f)
{
}

FEEGasDetEnergyReader::~FEEGasDetEnergyReader()
{
}

void   FEEGasDetEnergyReader::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   FEEGasDetEnergyReader::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t) 
{
  _add_to_cache("BLD:FEE:GDET1:PMT1:ENRC");
  _add_to_cache("BLD:FEE:GDET1:PMT2:ENRC");
  _add_to_cache("BLD:FEE:GDET2:PMT1:ENRC");
  _add_to_cache("BLD:FEE:GDET2:PMT2:ENRC");
  if (id.version() >= 1) {
    _add_to_cache("BLD:FEE:GDET2:PMT1:ENRC:SENSITIVE");
    _add_to_cache("BLD:FEE:GDET2:PMT2:ENRC:SENSITIVE");
  }
}

void   FEEGasDetEnergyReader::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    unsigned index = _index;
    switch(id.version()) {
    case 0:
      {
        const Pds::Bld::BldDataFEEGasDetEnergy& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataFEEGasDetEnergy*>(payload);
        _cache.cache(index++,bld.f_11_ENRC());
        _cache.cache(index++,bld.f_12_ENRC());
        _cache.cache(index++,bld.f_21_ENRC());
        _cache.cache(index++,bld.f_22_ENRC());
      }
    case 1:
      {
        const Pds::Bld::BldDataFEEGasDetEnergyV1& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataFEEGasDetEnergyV1*>(payload);
        _cache.cache(index++,bld.f_11_ENRC());
        _cache.cache(index++,bld.f_12_ENRC());
        _cache.cache(index++,bld.f_21_ENRC());
        _cache.cache(index++,bld.f_22_ENRC());
        _cache.cache(index++,bld.f_63_ENRC());
        _cache.cache(index  ,bld.f_64_ENRC());
      }
    }
  }
}

//  No Entry data
unsigned     FEEGasDetEnergyReader::nentries() const { return 0; }
const Entry* FEEGasDetEnergyReader::entry   (unsigned) const { return 0; }
void         FEEGasDetEnergyReader::rename  (const char* s) {}
