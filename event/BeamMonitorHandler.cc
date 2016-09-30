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
  strncpy(buffer,Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info())),54);
  char* iptr = buffer+strlen(buffer);

  switch(id.version()) {
  case 0:
    sprintf(iptr,":SUM");         _add_to_cache(buffer);
    sprintf(iptr,":XPOS");        _add_to_cache(buffer);
    sprintf(iptr,":YPOS");        _add_to_cache(buffer);
    sprintf(iptr,":PEAK:AMP");    _add_to_cache(buffer);
    sprintf(iptr,":PEAK:POS");    _add_to_cache(buffer);
    for(unsigned i=0; i<Pds::Bld::BldDataBeamMonitorV1::NCHANNELS; i++) {
      sprintf(iptr,":CH%02d",i);  _add_to_cache(buffer);
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
  case 0:
    { const Pds::Bld::BldDataBeamMonitorV1& d =
        *reinterpret_cast<const Pds::Bld::BldDataBeamMonitorV1*>(payload);
      _cache.cache(index++, d.TotalIntensity());
      _cache.cache(index++, d.X_Position());
      _cache.cache(index++, d.Y_Position());
      _cache.cache(index++, d.peakA());
      _cache.cache(index++, d.peakT());
      ndarray<const double, 1> chan_intesity = d.Channel_Intensity();
      for(unsigned i=0; i<Pds::Bld::BldDataBeamMonitorV1::NCHANNELS; i++) {
        _cache.cache(index++, chan_intesity[i]);
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
  strncpy(buffer,name,54);
  char* iptr = buffer+strlen(buffer);

  unsigned i=_index;

  sprintf(iptr,":SUM");       _rename_cache(i++,buffer);
  sprintf(iptr,":XPOS");      _rename_cache(i++,buffer);
  sprintf(iptr,":YPOS");      _rename_cache(i++,buffer);
  sprintf(iptr,":PEAK:AMP");  _rename_cache(i++,buffer);
  sprintf(iptr,":PEAK:POS");  _rename_cache(i++,buffer);

  for(; i<_indices.size(); i++) {
    sprintf(iptr,":CH%02d",i); _rename_cache(i,buffer);
  }
}
