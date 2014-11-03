#include "ControlXtcReader.hh"
#include "ami/data/FeatureCache.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/psddl/control.ddl.h"

#include <stdio.h>

using namespace Ami;

ControlXtcReader::ControlXtcReader(FeatureCache& f)  : 
  EventHandlerF(Pds::ProcInfo(Pds::Level::Control,0,unsigned(-1UL)),
		Pds::TypeId::NumberOf,
		Pds::TypeId::Id_ControlConfig,
		f),
  _cache(f),
  _values(0)
{
}

ControlXtcReader::~ControlXtcReader()
{
}

void   ControlXtcReader::_calibrate(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{ _configure(id, payload,t); }

void   ControlXtcReader::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  char nbuf[FeatureCache::FEATURE_NAMELEN];
  _index = -1;
  _values.resize(0);

  if (id.version()==Pds::ControlData::ConfigV1::Version) {
    const Pds::ControlData::ConfigV1& c =
      *reinterpret_cast<const Pds::ControlData::ConfigV1*>(payload);

    //  Add the PVControl variables
    for(unsigned k=0; k<c.npvControls(); k++) {
      const Pds::ControlData::PVControl& pv = c.pvControls()[k];
      if (pv.array())
        snprintf(nbuf, FeatureCache::FEATURE_NAMELEN, "%s[%d](SCAN)", pv.name(), pv.index());
      else
        snprintf(nbuf, FeatureCache::FEATURE_NAMELEN, "%s(SCAN)", pv.name());
      int index = _add_to_cache(nbuf);
      _cache.cache(index,pv.value());
      _values.push_back(pv.value());
    }
  }
  else if (id.version()==Pds::ControlData::ConfigV2::Version) {
    const Pds::ControlData::ConfigV2& c =
      *reinterpret_cast<const Pds::ControlData::ConfigV2*>(payload);

    //  Add the PVControl variables
    for(unsigned k=0; k<c.npvControls(); k++) {
      const Pds::ControlData::PVControl& pv = c.pvControls()[k];
      if (pv.array())
        snprintf(nbuf, FeatureCache::FEATURE_NAMELEN, "%s[%d](SCAN)", pv.name(), pv.index());
      else
        snprintf(nbuf, FeatureCache::FEATURE_NAMELEN, "%s(SCAN)", pv.name());
      int index = _add_to_cache(nbuf);
      _cache.cache(index,pv.value());
      _values.push_back(pv.value());
    }
  }
  else if (id.version()==Pds::ControlData::ConfigV3::Version) {
    const Pds::ControlData::ConfigV3& c =
      *reinterpret_cast<const Pds::ControlData::ConfigV3*>(payload);

    //  Add the PVControl variables
    for(unsigned k=0; k<c.npvControls(); k++) {
      const Pds::ControlData::PVControl& pv = c.pvControls()[k];
      if (pv.array())
        snprintf(nbuf, FeatureCache::FEATURE_NAMELEN, "%s[%d](SCAN)", pv.name(), pv.index());
      else
        snprintf(nbuf, FeatureCache::FEATURE_NAMELEN, "%s(SCAN)", pv.name());
      int index = _add_to_cache(nbuf);
      _cache.cache(index,pv.value());
      _values.push_back(pv.value());
    }
  }

  //
  //  Add an alias for each variable
  //
  unsigned nv=_values.size();
  for(unsigned i=0; i<nv; i++) {
    snprintf(nbuf, FeatureCache::FEATURE_NAMELEN, "XSCAN[%d]", i);
    _add_to_cache(nbuf);
    _values.push_back(_values[i]);
  }
}

//  no L1 data will appear from Control
void   ControlXtcReader::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t) 
{
  for(unsigned i=0, index=_index; i<_values.size(); i++,index++)
    _cache.cache(index,_values[i]);
}

void   ControlXtcReader::_damaged  () {}

//  No Entry data
unsigned     ControlXtcReader::nentries() const { return 0; }
const Entry* ControlXtcReader::entry   (unsigned) const { return 0; }
void         ControlXtcReader::rename  (const char*) {}
