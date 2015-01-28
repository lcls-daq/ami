#include "IpmFexHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/lusi.ddl.h"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

IpmFexHandler::IpmFexHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
		Pds::TypeId::Id_IpmFex,
		Pds::TypeId::Id_IpmFexConfig,
		f)
{
}

IpmFexHandler::~IpmFexHandler()
{
}

void   IpmFexHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   IpmFexHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  char* iptr;

  switch(info().level()) {
  case Level::Reporter:
    strncpy(buffer,Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info())),60);
    iptr = buffer+strlen(buffer);
    sprintf(iptr,":FEX:CH0");  _add_to_cache(buffer);
    sprintf(iptr,":FEX:CH1");  _add_to_cache(buffer);
    sprintf(iptr,":FEX:CH2");  _add_to_cache(buffer);
    sprintf(iptr,":FEX:CH3");  _add_to_cache(buffer);
    sprintf(iptr,":FEX:SUM");  _add_to_cache(buffer);
    sprintf(iptr,":FEX:XPOS"); _add_to_cache(buffer);
    sprintf(iptr,":FEX:YPOS"); _add_to_cache(buffer);
    break;
  case Level::Source:
  default:
    strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),60);
    iptr = buffer+strlen(buffer);
    sprintf(iptr,":CH0"); _add_to_cache(buffer);
    sprintf(iptr,":CH1"); _add_to_cache(buffer);
    sprintf(iptr,":CH2"); _add_to_cache(buffer);
    sprintf(iptr,":CH3"); _add_to_cache(buffer);
    sprintf(iptr,":SUM"); _add_to_cache(buffer);
    sprintf(iptr,":XPOS"); _add_to_cache(buffer);
    sprintf(iptr,":YPOS"); _add_to_cache(buffer);
    break;
  }
}

void   IpmFexHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::Lusi::IpmFexV1& d = *reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);

  unsigned i=_index;
  ndarray<const float,1> ch = d.channel();
  for(unsigned j=0; j<4; j++)
    _cache.cache(i++, ch[j]);
  _cache.cache(i++, d.sum() );
  _cache.cache(i++, d.xpos());
  _cache.cache(i++, d.ypos());
}

//  No Entry data
unsigned     IpmFexHandler::nentries() const { return 0; }
const Entry* IpmFexHandler::entry   (unsigned) const { return 0; }

void         IpmFexHandler::rename(const char* s)
{
  if (_index<0) return;

  unsigned i=_index;
  char buffer[64];
  strncpy(buffer,s,60);
  char* iptr = buffer+strlen(buffer);

  switch(info().level()) {
  case Level::Reporter:
    sprintf(iptr,":FEX:CH0");  _rename_cache(i++,buffer); 
    sprintf(iptr,":FEX:CH1");  _rename_cache(i++,buffer); 
    sprintf(iptr,":FEX:CH2");  _rename_cache(i++,buffer); 
    sprintf(iptr,":FEX:CH3");  _rename_cache(i++,buffer); 
    sprintf(iptr,":FEX:SUM");  _rename_cache(i++,buffer); 
    sprintf(iptr,":FEX:XPOS"); _rename_cache(i++,buffer); 
    sprintf(iptr,":FEX:YPOS"); _rename_cache(i++,buffer); 
    break;
  case Level::Source:
  default:
    sprintf(iptr,":CH0"); _rename_cache(i++,buffer); 
    sprintf(iptr,":CH1"); _rename_cache(i++,buffer); 
    sprintf(iptr,":CH2"); _rename_cache(i++,buffer); 
    sprintf(iptr,":CH3"); _rename_cache(i++,buffer); 
    sprintf(iptr,":SUM"); _rename_cache(i++,buffer); 
    sprintf(iptr,":XPOS"); _rename_cache(i++,buffer); 
    sprintf(iptr,":YPOS"); _rename_cache(i++,buffer); 
    break;
  }
}
