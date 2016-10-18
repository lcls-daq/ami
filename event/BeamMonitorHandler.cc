#include "BeamMonitorHandler.hh"

#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <math.h>
#include <stdio.h>

using namespace Ami;

BeamMonitorHandler::BeamMonitorHandler(const Pds::BldInfo& info, FeatureCache& f)  :
  EventHandlerF(info,
    Pds::TypeId::Id_BeamMonitorBldData,
    Pds::TypeId::Id_BeamMonitorBldData,
    f)
{
}

BeamMonitorHandler::~BeamMonitorHandler()
{
}

void   BeamMonitorHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   BeamMonitorHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  strncpy(buffer,Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info())),51);
  char* iptr = buffer+strlen(buffer);

  switch(id.version()) {
  case 1:
    sprintf(iptr,":SUM");         _add_to_cache(buffer);
    sprintf(iptr,":XPOS");        _add_to_cache(buffer);
    sprintf(iptr,":YPOS");        _add_to_cache(buffer);
    for(unsigned i=0; i<Pds::Bld::BldDataBeamMonitorV1::NCHANNELS; i++) {
      sprintf(iptr,":PEAK_A:CH%02d",i);  _add_to_cache(buffer);
      sprintf(iptr,":PEAK_T:CH%02d",i);  _add_to_cache(buffer);
    }
    break;
  default:
    break;
  }
}

void   BeamMonitorHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  unsigned index = _index;
  switch(id.version()) {
  case 1:
    { const Pds::Bld::BldDataBeamMonitorV1& d =
        *reinterpret_cast<const Pds::Bld::BldDataBeamMonitorV1*>(payload);
      _cache.cache(index++, d.TotalIntensity());
      _cache.cache(index++, d.X_Position());
      _cache.cache(index++, d.Y_Position());
      ndarray<const double, 1> peakA = d.peakA();
      ndarray<const unsigned short, 1> peakT = d.peakT();
      for(unsigned i=0; i<Pds::Bld::BldDataBeamMonitorV1::NCHANNELS; i++) {
        _cache.cache(index++, peakA[i]);
        _cache.cache(index++, peakT[i]);
      }
    } break;
  default:
    break;
  }
}

//  No Entry data
unsigned     BeamMonitorHandler::nentries() const { return 0; }
const Entry* BeamMonitorHandler::entry   (unsigned) const { return 0; }
void         BeamMonitorHandler::rename  (const char* name)
{
  char buffer[64];
  strncpy(buffer,name,51);
  char* iptr = buffer+strlen(buffer);

  unsigned index=_index;

  sprintf(iptr,":SUM");       _rename_cache(index++,buffer);
  sprintf(iptr,":XPOS");      _rename_cache(index++,buffer);
  sprintf(iptr,":YPOS");      _rename_cache(index++,buffer);
  for(unsigned i=0; index<_indices.size()-1; i++) {
    sprintf(iptr,":PEAK_A:CH%02d",i); _rename_cache(index++,buffer);
    sprintf(iptr,":PEAK_T:CH%02d",i); _rename_cache(index++,buffer);
  }
}
