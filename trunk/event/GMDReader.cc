#include "GMDReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>

using namespace Ami;

GMDReader::GMDReader(FeatureCache& f)  :
  EventHandlerF(Pds::BldInfo(0,Pds::BldInfo::GMD),
                Pds::TypeId::Id_GMD,
                Pds::TypeId::Id_GMD,
                f)
{
}

GMDReader::~GMDReader()
{
}

void   GMDReader::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   GMDReader::_configure(Pds::TypeId typeId, const void* payload, const Pds::ClockTime& t)
{
  if (typeId.version() == 1) {
    _add_to_cache("BLD:GMD:MilliJoulesPerPulse");
    _add_to_cache("BLD:GMD:MilliJoulesAverage");
    _add_to_cache("BLD:GMD:CorrectedSumPerPulse");
    _add_to_cache("BLD:GMD:BgValuePerSample");
    _add_to_cache("BLD:GMD:RelativeEnergyPerPulse");
  } else if (typeId.version() == 2) {
    _add_to_cache("BLD:GMD:MilliJoulesPerPulse");
    _add_to_cache("BLD:GMD:MilliJoulesAverage");
    _add_to_cache("BLD:GMD:SumAllPeaksFiltBkgd");
    _add_to_cache("BLD:GMD:RawAvgBkgd");
    _add_to_cache("BLD:GMD:RelativeEnergyPerPulse");
    _add_to_cache("BLD:GMD:SumAllPeaksRawBkgd");
  }
}

void   GMDReader::_event    (Pds::TypeId typeId, const void* payload, const Pds::ClockTime& t)
{
  if (_index <=0)
    return;

  if (typeId.version() == 1) {
    const Pds::Bld::BldDataGMDV1& bld =
      *reinterpret_cast<const Pds::Bld::BldDataGMDV1*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.milliJoulesPerPulse());
    _cache.cache(index++,bld.milliJoulesAverage());
    _cache.cache(index++,bld.correctedSumPerPulse());
    _cache.cache(index++,bld.bgValuePerSample());
    _cache.cache(index++,bld.relativeEnergyPerPulse());
  } else if (typeId.version() == 2) {
    const Pds::Bld::BldDataGMDV2& bld =
      *reinterpret_cast<const Pds::Bld::BldDataGMDV2*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.milliJoulesPerPulse());
    _cache.cache(index++,bld.milliJoulesAverage());
    _cache.cache(index++,bld.sumAllPeaksFiltBkgd());
    _cache.cache(index++,bld.rawAvgBkgd());
    _cache.cache(index++,bld.relativeEnergyPerPulse());
    _cache.cache(index++,bld.sumAllPeaksRawBkgd());
  }
}

//  No Entry data
unsigned     GMDReader::nentries() const { return 0; }
const Entry* GMDReader::entry   (unsigned) const { return 0; }
void         GMDReader::rename  (const char* s) {}
