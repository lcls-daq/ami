#include "BldXtcReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"

#include <stdio.h>

using namespace Ami;

BldXtcReader::BldXtcReader(FeatureCache& f)  : 
  EventHandler(Pds::BldInfo(0,Pds::BldInfo::FEEGasDetEnergy),
	       Pds::TypeId::Id_FEEGasDetEnergy,
	       Pds::TypeId::NumberOf),  // expect no configure
  _cache(f),
  _index(-1)
{
}

BldXtcReader::~BldXtcReader()
{
}

//  no configure data will appear from BLD
void   BldXtcReader::_calibrate(const void* payload, const Pds::ClockTime& t) {}
void   BldXtcReader::_configure(const void* payload, const Pds::ClockTime& t) {}

void   BldXtcReader::_event    (const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    const double* p = reinterpret_cast<const double*>(payload);
    unsigned index = _index;
    _cache.cache(index++,*p++);
    _cache.cache(index++,*p++);
    _cache.cache(index++,*p++);
    _cache.cache(index  ,*p  );
  }
}

void   BldXtcReader::_damaged  ()
{
  unsigned index = _index;
  _cache.cache(index++,-1,true);
  _cache.cache(index++,-1,true);
  _cache.cache(index++,-1,true);
  _cache.cache(index  ,-1,true);
}

//  No Entry data
unsigned     BldXtcReader::nentries() const { return 0; }
const Entry* BldXtcReader::entry   (unsigned) const { return 0; }
void         BldXtcReader::reset   () 
{
  _index = _cache.add("FEE:GDET:11:ENRC");
  _cache.add("FEE:GDET:12:ENRC");
  _cache.add("FEE:GDET:21:ENRC");
  _cache.add("FEE:GDET:22:ENRC");
}
