#include "EOrbitsHandler.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>

using namespace Ami;

EOrbitsHandler::EOrbitsHandler(const Pds::BldInfo& info, FeatureCache& f)  :
  EventHandlerF(info,
    Pds::TypeId::Id_EOrbits,
    Pds::TypeId::Id_EOrbits,
    f)
{
}

EOrbitsHandler::~EOrbitsHandler()
{
}

void   EOrbitsHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   EOrbitsHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  const char* name = "BLD:EORBITS";
  char* buffer = new char[strlen(name)+15];
  strcpy(buffer,name);
  char* iptr = buffer+strlen(buffer);

  switch(id.version()) {
  case 0:
    { const Pds::Bld::BldDataEOrbitsV0& d = *reinterpret_cast<const Pds::Bld::BldDataEOrbitsV0*>(payload);
      for(unsigned i=0; i<d.nBPMS(); i++) {
        sprintf(iptr,":BPMX:CH%d",i);
        _add_to_cache(buffer);
        sprintf(iptr,":BPMY:CH%d",i);
        _add_to_cache(buffer);
        sprintf(iptr,":BPMTMIT:CH%03d",i);
        _add_to_cache(buffer);
      }
    } break;
  default:
    break;
  }

  delete[] buffer;
}

void   EOrbitsHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  if (_index>=0) {
    unsigned index = _index;
    switch(id.version()) {
    case 0:
      { const Pds::Bld::BldDataEOrbitsV0& d =
          *reinterpret_cast<const Pds::Bld::BldDataEOrbitsV0*>(payload);
        ndarray<const double, 1> bpm_x    = d.fBPM_X();
        ndarray<const double, 1> bpm_y    = d.fBPM_Y();
        ndarray<const double, 1> bpm_tmit = d.fBPM_TMIT();
        for(unsigned i=0; i<d.nBPMS(); i++) {
          _cache.cache(index++,bpm_x[i]);
          _cache.cache(index++,bpm_y[i]);
          _cache.cache(index++,bpm_tmit[i]);
        }
      } break;
    default:
      break;
    }
  }
}

//  No Entry data
unsigned     EOrbitsHandler::nentries() const { return 0; }
const Entry* EOrbitsHandler::entry   (unsigned) const { return 0; }
void         EOrbitsHandler::rename  (const char* name)
{
  char* buffer = new char[strlen(name)+15];
  strcpy(buffer,name);
  char* iptr = buffer+strlen(buffer);

  for(unsigned i=0; i<_indices.size(); i++) {
    switch(i % 3) {
    case 0:
      sprintf(iptr,":BPMX:CH%d",i);
      _rename_cache(_indices[i],buffer);
      break;
    case 1:
      sprintf(iptr,":BPMY:CH%d",i);
      _rename_cache(_indices[i],buffer);
      break;
    case 2:
      sprintf(iptr,":BPMTMIT:CH%03d",i);
      _rename_cache(_indices[i],buffer);
      break;
    }
  }

  delete[] buffer;
}
