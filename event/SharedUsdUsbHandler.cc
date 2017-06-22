#include "SharedUsdUsbHandler.hh"

#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>
#include <string.h>

using namespace Ami;

SharedUsdUsbHandler::SharedUsdUsbHandler(const Pds::BldInfo& bldInfo, FeatureCache& f) :
  EventHandlerF(bldInfo, Pds::TypeId::Id_SharedUsdUsb, Pds::TypeId::Id_SharedUsdUsb, f),
  _alias_mask(0)
{
}

SharedUsdUsbHandler::~SharedUsdUsbHandler()
{
}

void SharedUsdUsbHandler::rename(const char* s)
{
  if (_index<0) return;

  char buffer[NAME_LEN];
  snprintf(buffer,NAME_LEN-8,s);
  char* c = buffer+strlen(buffer);

  unsigned index(_index);
  sprintf(c,":TS"); _rename_cache(index,buffer); index++;
  for(unsigned i=0; i<NCHAN; i++) {
    sprintf(c,":CH%d",i);
    _rename_cache(index,buffer); index++;
  }
  for(unsigned i=0; i<NCHAN; i++) {
    sprintf(c,":AIN%d",i);
    _rename_cache(index,buffer); index++;
  }
  for(unsigned i=0; i<DCHAN; i++) {
    sprintf(c,":DIN%d",i);
    _rename_cache(index,buffer); index++;
  }
  for(unsigned i=0; i<NCHAN; i++) {
    sprintf(c,":FEX:CH%d",i);
    _rename_cache(index,buffer); index++;
    if (_use_alias(i)) {
      _rename_cache(index,_aliases[i]); index++;
    }
  }
}

void SharedUsdUsbHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void SharedUsdUsbHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  char buffer[NAME_LEN];
  strncpy(buffer,Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info())),NAME_LEN-8);
  char* c = buffer+strlen(buffer);

  // reset alias mask
  _alias_mask = 0;

  switch(id.version()) {
  case 1: {
    const Pds::UsdUsb::FexConfigV1& cfg = reinterpret_cast<const Pds::Bld::BldDataUsdUsbV1*>(payload)->fexConfig();
    sprintf(c,":TS");
    _add_to_cache(buffer);
    for(unsigned i=0; i<NCHAN; i++) {
      sprintf(c,":CH%d",i);
      _add_to_cache(buffer);
    }
    for(unsigned i=0; i<NCHAN; i++) {
      sprintf(c,":AIN%d",i);
      _add_to_cache(buffer);
    }
    for(unsigned i=0; i<DCHAN; i++) {
      sprintf(c,":DIN%d",i);
      _add_to_cache(buffer);
    }
    for(unsigned i=0; i<NCHAN; i++) {
      sprintf(c,":FEX:CH%d",i);
      _add_to_cache(buffer);
      // grab aliases from cfg
      strncpy(_aliases[i],cfg.name(i),NAME_LEN);
      if (strlen(_aliases[i]) > 0) {
        if(_cache.lookup(_aliases[i])==-1) {
          _add_to_cache(_aliases[i]);
          _alias_mask |= (1<<i);
        } else {
          printf("UsdUsbFexHandler::_configure: requested alias for channel %d already in use: %s\n", i, _aliases[i]);
        }
      }
    }
  } break;
  default:
    break;
  }
}

void SharedUsdUsbHandler::_event(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  switch(id.version()) {
  case 1: {
    const Pds::Bld::BldDataUsdUsbV1& b = *reinterpret_cast<const Pds::Bld::BldDataUsdUsbV1*>(payload);
    const Pds::UsdUsb::DataV1& d = b.data();
    const Pds::UsdUsb::FexDataV1& f = b.fexData();
    unsigned index = _index;
    _cache.cache(index++, d.timestamp());
    for(unsigned i=0; i<NCHAN; i++)
      _cache.cache(index++, d.encoder_count()[i]);
    for(unsigned i=0; i<NCHAN; i++)
      _cache.cache(index++, d.analog_in()[i]);
    unsigned din = d.digital_in();
    for(unsigned i=0; i<DCHAN; i++)
      _cache.cache(index++, (din>>i)&1);
    for(unsigned i=0; i<NCHAN; i++) {
      _cache.cache(index++, f.encoder_values()[i]);
      if (_use_alias(i))
        _cache.cache(index++, f.encoder_values()[i]);
    }
  } break;
  default:
    break;
  }
}

bool SharedUsdUsbHandler::_use_alias(const unsigned index) const
{
  return _alias_mask & (1<<index);
}

//  No Entry data
unsigned     SharedUsdUsbHandler::nentries() const { return 0; }
const Entry* SharedUsdUsbHandler::entry   (unsigned) const { return 0; }
