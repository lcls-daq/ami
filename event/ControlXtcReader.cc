#include "ControlXtcReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/control/ConfigV1.hh"
#include "pdsdata/control/PVControl.hh"

#include <stdio.h>

using namespace Ami;

ControlXtcReader::ControlXtcReader(FeatureCache& f)  : 
  EventHandler(Pds::ProcInfo(Pds::Level::Control,0,-1UL),
	       Pds::TypeId::NumberOf,   // expect no L1 data
	       Pds::TypeId::Id_ControlConfig),  // expect no configure
  _cache(f),
  _index(-1)
{
}

ControlXtcReader::~ControlXtcReader()
{
}

void   ControlXtcReader::_configure(const void* payload)
{
  const Pds::ControlData::ConfigV1& c =
    *reinterpret_cast<const Pds::ControlData::ConfigV1*>(payload);

  //  Add the PVControl variables
  char nbuf[FeatureCache::FEATURE_NAMELEN];
  for(unsigned k=0; k<c.npvControls(); k++) {
    const Pds::ControlData::PVControl& pv = c.pvControl(k);
    int index;
    if (pv.array()) {
      snprintf(nbuf, FeatureCache::FEATURE_NAMELEN, "%s[%d]", pv.name(), pv.index());
      index = _cache.add(nbuf);
    }
    else
      index = _cache.add(pv.name());
    _cache.cache(index,pv.value());
  }
}

//  no L1 data will appear from Control
void   ControlXtcReader::_event    (const void* payload) {}

void   ControlXtcReader::_damaged  () {}

//  No Entry data
unsigned     ControlXtcReader::nentries() const { return 0; }
const Entry* ControlXtcReader::entry   (unsigned) const { return 0; }
void         ControlXtcReader::reset   () {}
