#include "TimeToolHandler.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pdsdata/psddl/timetool.ddl.h"

#include <stdio.h>

static const char* _scalar_name[] = { "AMPL",
				      "FLTPOS",
				      "FLTPOS_PS",
				      "FLTPOSFWHM",
				      "AMPLNXT",
				      "REFAMPL",
				      NULL };

Ami::TimeToolHandler::TimeToolHandler(const Pds::Src& info, Ami::FeatureCache& f)  : 
  Ami::EventHandlerF(info,
		     Pds::TypeId::Id_TimeToolConfig,
		     Pds::TypeId::Id_TimeToolData,
		     f)
{
  reset();
}

Ami::TimeToolHandler::~TimeToolHandler()
{
}

void   Ami::TimeToolHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   Ami::TimeToolHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  switch(id.version()) {
  case 1:
    { const Pds::TimeTool::ConfigV1& c = 
	*reinterpret_cast<const Pds::TimeTool::ConfigV1*>(payload);

      char buffer[64];
      for(unsigned i=0; i<MaxScalars; i++) {
	sprintf(buffer,"%s:%s",c.base_name(),_scalar_name[i]);
	_index[i] = _add_to_cache(buffer);
      }
    } break;
  default:
    break;
  };
}

void   Ami::TimeToolHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  switch(id.version()) {
  case 1:
    { const Pds::TimeTool::DataV1& d = 
	*reinterpret_cast<const Pds::TimeTool::DataV1*>(payload);

      _cache.cache(_index[0],d.amplitude());
      _cache.cache(_index[1],d.position_pixel());
      _cache.cache(_index[2],d.position_time());
      _cache.cache(_index[3],d.position_fwhm());
      _cache.cache(_index[4],d.nxt_amplitude());
      _cache.cache(_index[5],d.ref_amplitude());
    } break;
  default:
    break;
  }
}

void   Ami::TimeToolHandler::_damaged  ()
{
  for(unsigned i=0; i<MaxScalars; i++)
    if (_index[i]>=0)
      _cache.cache(_index[i], 0, true);
}

//  No Entry data
unsigned          Ami::TimeToolHandler::nentries() const { return 0; }
const Ami::Entry* Ami::TimeToolHandler::entry   (unsigned) const { return 0; }
void              Ami::TimeToolHandler::reset   () 
{
  EventHandlerF::reset();
  for(unsigned i=0; i<MaxScalars; i++)
    _index[i] = -1;
}
void              Ami::TimeToolHandler::rename(const char*) {}
