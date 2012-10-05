#include "UsdUsbHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/usdusb/ConfigV1.hh"
#include "pdsdata/usdusb/DataV1.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

UsdUsbHandler::UsdUsbHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandler(info,
	       Pds::TypeId::Id_UsdUsbData,
	       Pds::TypeId::Id_UsdUsbConfig),
  _cache(f)
{
}

UsdUsbHandler::~UsdUsbHandler()
{
}

void   UsdUsbHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   UsdUsbHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),59);
  char* c = buffer+strlen(buffer);

  switch(id.version()) {
  case 1: {
    sprintf(c,":TS");
    _index = _cache.add(buffer);
    for(unsigned i=0; i<4; i++) {
      sprintf(c,":CH%d",i);
      _cache.add(buffer);
    }
    for(unsigned i=0; i<4; i++) {
      sprintf(c,":AIN%d",i);
      _cache.add(buffer);
    }
    for(unsigned i=0; i<8; i++) {
      sprintf(c,":DIN%d",i);
      _cache.add(buffer);
    }
  } break;
  default:
    break;
  }
}

void   UsdUsbHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  switch(id.version()) {
  case 1: {
    const Pds::UsdUsb::DataV1& d = *reinterpret_cast<const Pds::UsdUsb::DataV1*>(payload);
    unsigned index = _index;
    _cache.cache(index++, d.timestamp());
    for(unsigned i=0; i<4; i++)
      _cache.cache(index++, d.encoder_count(i));
    for(unsigned i=0; i<4; i++)
      _cache.cache(index++, d.analog_in(i));
    unsigned din = d.digital_in();
    for(unsigned i=0; i<8; i++)
      _cache.cache(index++, (din>>i)&1);
  } break;
  default:
    break;
  }
}

void   UsdUsbHandler::_damaged  ()
{
  for(unsigned i=0; i<17; i++)
    _cache.cache(_index+i,0,true);
}

//  No Entry data
unsigned     UsdUsbHandler::nentries() const { return 0; }
const Entry* UsdUsbHandler::entry   (unsigned) const { return 0; }
void         UsdUsbHandler::reset   () 
{
}
