#include "EncoderHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/encoder.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

EncoderHandler::EncoderHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
		Pds::TypeId::Id_EncoderData,
		Pds::TypeId::Id_EncoderConfig,
		f)
{
}

EncoderHandler::~EncoderHandler()
{
}

void   EncoderHandler::reset () 
{
  EventHandlerF::reset();
  _index=-1; 
}

void   EncoderHandler::rename(const char* s)
{
  char buffer[64];
  unsigned index(_index);
  for(unsigned i=0; i<3; i++,index++) {
    sprintf(buffer,"%s:CH%d",s,i);
    _rename_cache(index,buffer);
  }
}

void   EncoderHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   EncoderHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),59);
  char* c = buffer+strlen(buffer);

  sprintf(c,":CH0");
  _index = _add_to_cache(buffer);
  sprintf(c,":CH1");
  _add_to_cache(buffer);
  sprintf(c,":CH2");
  _add_to_cache(buffer);
}

void   EncoderHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::Encoder::DataV2& d = *reinterpret_cast<const Pds::Encoder::DataV2*>(payload);
  _cache.cache(_index+0, d.value(0));
  _cache.cache(_index+1, d.value(1));
  _cache.cache(_index+2, d.value(2));
}

void   EncoderHandler::_damaged  ()
{
  _cache.cache(_index, 0, true);
}

//  No Entry data
unsigned     EncoderHandler::nentries() const { return 0; }
const Entry* EncoderHandler::entry   (unsigned) const { return 0; }
