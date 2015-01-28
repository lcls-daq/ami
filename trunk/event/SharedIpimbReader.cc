#include "SharedIpimbReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>
#include <string.h>

using namespace Ami;

SharedIpimbReader::SharedIpimbReader(const Pds::BldInfo& bldInfo, FeatureCache& f)  : 
  EventHandlerF(bldInfo, Pds::TypeId::Id_SharedIpimb, Pds::TypeId::Id_SharedIpimb, f)
{
}

SharedIpimbReader::~SharedIpimbReader()
{
}

void  SharedIpimbReader::_calibrate(Pds::TypeId id, const void* payload, const Pds::ClockTime& t) {}
void  SharedIpimbReader::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t) 
{
  char buffer[64];
  strncpy(buffer,Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(info())),60);
  char* iptr = buffer+strlen(buffer);
  
  sprintf(iptr,":DATA[0]");  _add_to_cache(buffer);  
  sprintf(iptr,":DATA[1]");  _add_to_cache(buffer);  
  sprintf(iptr,":DATA[2]");  _add_to_cache(buffer);  
  sprintf(iptr,":DATA[3]");  _add_to_cache(buffer);  

  sprintf(iptr,":FEX:CH0");  _add_to_cache(buffer);  
  sprintf(iptr,":FEX:CH1");  _add_to_cache(buffer);  
  sprintf(iptr,":FEX:CH2");  _add_to_cache(buffer);  
  sprintf(iptr,":FEX:CH3");  _add_to_cache(buffer);  
  
  sprintf(iptr,":FEX:SUM");  _add_to_cache(buffer);  
  sprintf(iptr,":FEX:XPOS"); _add_to_cache(buffer);  
  sprintf(iptr,":FEX:YPOS"); _add_to_cache(buffer);  

  if (id.version()==1) {
    sprintf(iptr,":PS:DATA[0]");  _add_to_cache(buffer);  
    sprintf(iptr,":PS:DATA[1]");  _add_to_cache(buffer);  
    sprintf(iptr,":PS:DATA[2]");  _add_to_cache(buffer);  
    sprintf(iptr,":PS:DATA[3]");  _add_to_cache(buffer);  
  }
}

void SharedIpimbReader::_event (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  int index=_index;
  switch(id.version()) {
  case 0:
    { const Pds::Bld::BldDataIpimbV0& bld = 
        *reinterpret_cast<const Pds::Bld::BldDataIpimbV0*>(payload);

      _cache.cache(index++, bld.ipimbData().channel0Volts());
      _cache.cache(index++, bld.ipimbData().channel1Volts());
      _cache.cache(index++, bld.ipimbData().channel2Volts());
      _cache.cache(index++, bld.ipimbData().channel3Volts());
      
      _cache.cache(index++, bld.ipmFexData().channel()[0]);
      _cache.cache(index++, bld.ipmFexData().channel()[1]);
      _cache.cache(index++, bld.ipmFexData().channel()[2]);
      _cache.cache(index++, bld.ipmFexData().channel()[3]);
  
      _cache.cache(index++, bld.ipmFexData().sum ());
      _cache.cache(index++, bld.ipmFexData().xpos());
      _cache.cache(index++,bld.ipmFexData().ypos());
    } break;
  case 1:
    { const Pds::Bld::BldDataIpimbV1& bld = 
        *reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload);

      _cache.cache(index++, bld.ipimbData().channel0Volts());
      _cache.cache(index++, bld.ipimbData().channel1Volts());
      _cache.cache(index++, bld.ipimbData().channel2Volts());
      _cache.cache(index++, bld.ipimbData().channel3Volts());
      
      _cache.cache(index++, bld.ipmFexData().channel()[0]);
      _cache.cache(index++, bld.ipmFexData().channel()[1]);
      _cache.cache(index++, bld.ipmFexData().channel()[2]);
      _cache.cache(index++, bld.ipmFexData().channel()[3]);
  
      _cache.cache(index++, bld.ipmFexData().sum ());
      _cache.cache(index++, bld.ipmFexData().xpos());
      _cache.cache(index++,bld.ipmFexData().ypos());

      _cache.cache(index++, bld.ipimbData().channel0psVolts());
      _cache.cache(index++, bld.ipimbData().channel1psVolts());
      _cache.cache(index++, bld.ipimbData().channel2psVolts());
      _cache.cache(index++, bld.ipimbData().channel3psVolts());
    } break;
  default:
    break;
  }  
}

//  No Entry data
unsigned     SharedIpimbReader::nentries() const { return 0; }
const Entry* SharedIpimbReader::entry   (unsigned) const { return 0; }
void         SharedIpimbReader::rename  (const char* s)
{
  if (_index<0) return;

  char buffer[64];
  strncpy(buffer,s,60);
  char* iptr = buffer+strlen(buffer);
  
  int i=_index;
  sprintf(iptr,":DATA[0]");  _rename_cache(i++,buffer); 
  sprintf(iptr,":DATA[1]");  _rename_cache(i++,buffer); 
  sprintf(iptr,":DATA[2]");  _rename_cache(i++,buffer); 
  sprintf(iptr,":DATA[3]");  _rename_cache(i++,buffer); 

  sprintf(iptr,":FEX:CH0");  _rename_cache(i++,buffer); 
  sprintf(iptr,":FEX:CH1");  _rename_cache(i++,buffer); 
  sprintf(iptr,":FEX:CH2");  _rename_cache(i++,buffer); 
  sprintf(iptr,":FEX:CH3");  _rename_cache(i++,buffer); 
  
  sprintf(iptr,":FEX:SUM");  _rename_cache(i++,buffer); 
  sprintf(iptr,":FEX:XPOS"); _rename_cache(i++,buffer); 
  sprintf(iptr,":FEX:YPOS"); _rename_cache(i++,buffer); 

  sprintf(iptr,":PS:DATA[0]");  _rename_cache(i++,buffer); 
  sprintf(iptr,":PS:DATA[1]");  _rename_cache(i++,buffer); 
  sprintf(iptr,":PS:DATA[2]");  _rename_cache(i++,buffer); 
  sprintf(iptr,":PS:DATA[3]");  _rename_cache(i++,buffer); 
}
