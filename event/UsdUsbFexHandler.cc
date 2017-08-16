#include "UsdUsbFexHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/usdusb.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;
using namespace Pds;

UsdUsbFexHandler::UsdUsbFexHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
                Pds::TypeId::Id_UsdUsbFexData,
                Pds::TypeId::Id_UsdUsbFexConfig,
                f),
  _alias_mask(0)
{
}

UsdUsbFexHandler::~UsdUsbFexHandler()
{
}

void         UsdUsbFexHandler::rename  (const char* s)
{
  if (_index<0) return;

  char buffer[NAME_LEN];
  snprintf(buffer,NAME_LEN-8,s);
  char* c = buffer+strlen(buffer);
  
  unsigned index(_index);
  for(unsigned i=0; i<NCHAN; i++) {
    sprintf(c,":FEX:CH%d",i);
    _rename_cache(index,buffer); index++;
    if (_use_alias(i)) {
      _rename_cache(index,_aliases[i]); index++;
    }
  }
}

void   UsdUsbFexHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   UsdUsbFexHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  char buffer[NAME_LEN];
  strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),NAME_LEN-8);
  char* c = buffer+strlen(buffer);

  // reset alias mask
  _alias_mask = 0;

  switch(id.version()) {
  case 1: {
    const Pds::UsdUsb::FexConfigV1& cfg = *reinterpret_cast<const Pds::UsdUsb::FexConfigV1*>(payload);
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

void   UsdUsbFexHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  switch(id.version()) {
  case 1: {
    const Pds::UsdUsb::FexDataV1& d = *reinterpret_cast<const Pds::UsdUsb::FexDataV1*>(payload);
    unsigned index = _index;
    for(unsigned i=0; i<NCHAN; i++) {
      _cache.cache(index++, d.encoder_values()[i]);
      if (_use_alias(i))
        _cache.cache(index++, d.encoder_values()[i]);
    }
  } break;
  default:
    break;
  }
}

bool   UsdUsbFexHandler::_use_alias(const unsigned index) const
{
  return _alias_mask & (1<<index);
}

//  No Entry data
unsigned     UsdUsbFexHandler::nentries() const { return 0; }
const Entry* UsdUsbFexHandler::entry   (unsigned) const { return 0; }
