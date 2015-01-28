#include "TimeToolHandler.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pdsdata/psddl/timetool.ddl.h"

#include <stdio.h>

enum { MaxScalars=6 };

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
      for(unsigned i=0; _scalar_name[i]!=NULL; i++) {
	sprintf(buffer,"%s:%s",c.base_name(),_scalar_name[i]);
	_add_to_cache(buffer);
      }
    } break;
  case 2:
    { const Pds::TimeTool::ConfigV2& c = 
	*reinterpret_cast<const Pds::TimeTool::ConfigV2*>(payload);

      char buffer[64];
      for(unsigned i=0; _scalar_name[i]!=NULL; i++) {
	sprintf(buffer,"%s:%s",c.base_name(),_scalar_name[i]);
	_add_to_cache(buffer);
      }
    } break;
  default:
    break;
  };
}

void   Ami::TimeToolHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  int index=_index;
  switch(id.version()) {
  case 1:
    { const Pds::TimeTool::DataV1& d = 
	*reinterpret_cast<const Pds::TimeTool::DataV1*>(payload);
      
      _cache.cache(index++,d.amplitude());
      _cache.cache(index++,d.position_pixel());
      _cache.cache(index++,d.position_time());
      _cache.cache(index++,d.position_fwhm());
      _cache.cache(index++,d.nxt_amplitude());
      _cache.cache(index++,d.ref_amplitude());
    } break;
  case 2:
    { const Pds::TimeTool::DataV2& d = 
	*reinterpret_cast<const Pds::TimeTool::DataV2*>(payload);
      
      _cache.cache(index++,d.amplitude());
      _cache.cache(index++,d.position_pixel());
      _cache.cache(index++,d.position_time());
      _cache.cache(index++,d.position_fwhm());
      _cache.cache(index++,d.nxt_amplitude());
      _cache.cache(index++,d.ref_amplitude());
    } break;
  default:
    break;
  }
}

//  No Entry data
unsigned          Ami::TimeToolHandler::nentries() const { return 0; }
const Ami::Entry* Ami::TimeToolHandler::entry   (unsigned) const { return 0; }
void              Ami::TimeToolHandler::rename(const char*) {}
