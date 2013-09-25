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
  unsigned i=0;

  switch(info().level()) {
  case Level::Reporter:
    strncpy(buffer,Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info())),60);
    iptr = buffer+strlen(buffer);
    sprintf(iptr,":FEX:CH0");  _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":FEX:CH1");  _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":FEX:CH2");  _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":FEX:CH3");  _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":FEX:SUM");  _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":FEX:XPOS"); _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":FEX:YPOS"); _index[i] = _add_to_cache(buffer);  i++;
    break;
  case Level::Source:
  default:
    strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),60);
    iptr = buffer+strlen(buffer);
    sprintf(iptr,":CH0"); _index[i++] = _add_to_cache(buffer);
    sprintf(iptr,":CH1"); _index[i++] = _add_to_cache(buffer);
    sprintf(iptr,":CH2"); _index[i++] = _add_to_cache(buffer);
    sprintf(iptr,":CH3"); _index[i++] = _add_to_cache(buffer);
    sprintf(iptr,":SUM");
    _index[i++] = _add_to_cache(buffer);
    sprintf(iptr,":XPOS");
    _index[i++] = _add_to_cache(buffer);
    sprintf(iptr,":YPOS");
    _index[i++] = _add_to_cache(buffer);
    break;
  }
}

void   IpmFexHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::Lusi::IpmFexV1& d = *reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);

  unsigned i=0;
  ndarray<const float,1> ch = d.channel();
  _cache.cache(_index[i], ch[i]); i++;
  _cache.cache(_index[i], ch[i]); i++;
  _cache.cache(_index[i], ch[i]); i++;
  _cache.cache(_index[i], ch[i]); i++;
  _cache.cache(_index[i], d.sum() ); i++;
  _cache.cache(_index[i], d.xpos()); i++;
  _cache.cache(_index[i], d.ypos()); i++;
}

void   IpmFexHandler::_damaged  ()
{
  for(unsigned i=0; i<NChannels; i++)
    _cache.cache(_index[i], 0, true);
}

//  No Entry data
unsigned     IpmFexHandler::nentries() const { return 0; }
const Entry* IpmFexHandler::entry   (unsigned) const { return 0; }

void         IpmFexHandler::rename(const char* s)
{
  char buffer[64];
  char* iptr;
  unsigned i=0;

  switch(info().level()) {
  case Level::Reporter:
    strncpy(buffer,s,60);
    iptr = buffer+strlen(buffer);
    sprintf(iptr,":FEX:CH0");  _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":FEX:CH1");  _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":FEX:CH2");  _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":FEX:CH3");  _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":FEX:SUM");  _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":FEX:XPOS"); _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":FEX:YPOS"); _rename_cache(_index[i],buffer); i++;
    break;
  case Level::Source:
  default:
    strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),60);
    iptr = buffer+strlen(buffer);
    sprintf(iptr,":CH0"); _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":CH1"); _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":CH2"); _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":CH3"); _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":SUM"); _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":XPOS"); _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":YPOS"); _rename_cache(_index[i],buffer); i++;
    break;
  }
}
