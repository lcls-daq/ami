#include "DiodeFexHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/lusi.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

DiodeFexHandler::DiodeFexHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
		Pds::TypeId::Id_DiodeFex,
		Pds::TypeId::Id_DiodeFexConfig,
		f)
{
}

DiodeFexHandler::~DiodeFexHandler()
{
}

void   DiodeFexHandler::rename(const char* s)
{
  _rename_cache(_index,s);
}

void   DiodeFexHandler::reset() 
{
  EventHandlerF::reset();
  _index=-1; 
}

void   DiodeFexHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   DiodeFexHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];

  strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),60);
  char* iptr = buffer+strlen(buffer);
  
  sprintf(iptr,":CH0"); 
  _index = _add_to_cache(buffer);
}

void   DiodeFexHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::Lusi::DiodeFexV1& d = *reinterpret_cast<const Pds::Lusi::DiodeFexV1*>(payload);

  _cache.cache(_index, d.value());
}

void   DiodeFexHandler::_damaged  ()
{
  _cache.cache(_index, 0, true);
}

//  No Entry data
unsigned     DiodeFexHandler::nentries() const { return 0; }
const Entry* DiodeFexHandler::entry   (unsigned) const { return 0; }
