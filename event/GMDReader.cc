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
  _index =
  _cache.add("BLD:GMD:MilliJoulesPerPulse");
  _cache.add("BLD:GMD:MilliJoulesAverage");
  _cache.add("BLD:GMD:CorrectedSumPerPulse");
  _cache.add("BLD:GMD:BgValuePerSample");
  _cache.add("BLD:GMD:RelativeEnergyPerPulse");
  _cache.add("BLD:GMD:Spare1");
}

void   GMDReader::_event    (const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const Pds::BldDataGMD& bld = 
      *reinterpret_cast<const Pds::BldDataGMD*>(payload);
    unsigned index = _index;
    _cache.cache(index++,bld.fMilliJoulesPerPulse);
    _cache.cache(index++,bld.fMilliJoulesAverage);
    _cache.cache(index++,bld.fCorrectedSumPerPulse);
    _cache.cache(index++,bld.fBgValuePerSample);
    _cache.cache(index++,bld.fRelativeEnergyPerPulse);
    _cache.cache(index++,bld.fSpare1);
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
void         GMDReader::reset   () { _index=-1; }
