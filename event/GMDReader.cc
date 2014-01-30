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
                f),
  _cache(f),
  _index(-1)
{
}

GMDReader::~GMDReader()
{
}

void   GMDReader::reset()
{
  EventHandlerF::reset();
  _index=-1;
}

void   GMDReader::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   GMDReader::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t) 
{
  _index =
  _cache.add("BLD:GMD:MilliJoulesPerPulse");
  _cache.add("BLD:GMD:MilliJoulesAverage");
  _cache.add("BLD:GMD:CorrectedSumPerPulse");
  _cache.add("BLD:GMD:BgValuePerSample");
  _cache.add("BLD:GMD:RelativeEnergyPerPulse");
}

void   GMDReader::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const Pds::Bld::BldDataGMDV1& bld = 
      *reinterpret_cast<const Pds::Bld::BldDataGMDV1*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.milliJoulesPerPulse());
    _cache.cache(index++,bld.milliJoulesAverage());
    _cache.cache(index++,bld.correctedSumPerPulse());
    _cache.cache(index++,bld.bgValuePerSample());
    _cache.cache(index++,bld.relativeEnergyPerPulse());
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
  }
}

//  No Entry data
unsigned     GMDReader::nentries() const { return 0; }
const Entry* GMDReader::entry   (unsigned) const { return 0; }
void         GMDReader::rename  (const char* s) {}
