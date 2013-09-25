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
  
  unsigned i=0;
  sprintf(iptr,":DATA[0]");  _index[i] = _add_to_cache(buffer);  i++;
  sprintf(iptr,":DATA[1]");  _index[i] = _add_to_cache(buffer);  i++;
  sprintf(iptr,":DATA[2]");  _index[i] = _add_to_cache(buffer);  i++;
  sprintf(iptr,":DATA[3]");  _index[i] = _add_to_cache(buffer);  i++;

  sprintf(iptr,":FEX:CH0");  _index[i] = _add_to_cache(buffer);  i++;
  sprintf(iptr,":FEX:CH1");  _index[i] = _add_to_cache(buffer);  i++;
  sprintf(iptr,":FEX:CH2");  _index[i] = _add_to_cache(buffer);  i++;
  sprintf(iptr,":FEX:CH3");  _index[i] = _add_to_cache(buffer);  i++;
  
  sprintf(iptr,":FEX:SUM");  _index[i] = _add_to_cache(buffer);  i++;
  sprintf(iptr,":FEX:XPOS"); _index[i] = _add_to_cache(buffer);  i++;
  sprintf(iptr,":FEX:YPOS"); _index[i] = _add_to_cache(buffer);  i++;

  if (id.version()==1) {
    sprintf(iptr,":PS:DATA[0]");  _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":PS:DATA[1]");  _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":PS:DATA[2]");  _index[i] = _add_to_cache(buffer);  i++;
    sprintf(iptr,":PS:DATA[3]");  _index[i] = _add_to_cache(buffer);  i++;
  }
  else
    for(unsigned j=0; j<4; j++)
      _index[i++] = _index[0];
}

void SharedIpimbReader::_event (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  switch(id.version()) {
  case 0:
    { const Pds::Bld::BldDataIpimbV0& bld = 
        *reinterpret_cast<const Pds::Bld::BldDataIpimbV0*>(payload);

      _cache.cache(_index[0], bld.ipimbData().channel0Volts());
      _cache.cache(_index[1], bld.ipimbData().channel1Volts());
      _cache.cache(_index[2], bld.ipimbData().channel2Volts());
      _cache.cache(_index[3], bld.ipimbData().channel3Volts());
      
      _cache.cache(_index[4], bld.ipmFexData().channel()[0]);
      _cache.cache(_index[5], bld.ipmFexData().channel()[1]);
      _cache.cache(_index[6], bld.ipmFexData().channel()[2]);
      _cache.cache(_index[7], bld.ipmFexData().channel()[3]);
  
      _cache.cache(_index[8], bld.ipmFexData().sum ());
      _cache.cache(_index[9], bld.ipmFexData().xpos());
      _cache.cache(_index[10],bld.ipmFexData().ypos());
    } break;
  case 1:
    { const Pds::Bld::BldDataIpimbV1& bld = 
        *reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload);

      _cache.cache(_index[0], bld.ipimbData().channel0Volts());
      _cache.cache(_index[1], bld.ipimbData().channel1Volts());
      _cache.cache(_index[2], bld.ipimbData().channel2Volts());
      _cache.cache(_index[3], bld.ipimbData().channel3Volts());
      
      _cache.cache(_index[4], bld.ipmFexData().channel()[0]);
      _cache.cache(_index[5], bld.ipmFexData().channel()[1]);
      _cache.cache(_index[6], bld.ipmFexData().channel()[2]);
      _cache.cache(_index[7], bld.ipmFexData().channel()[3]);
  
      _cache.cache(_index[8], bld.ipmFexData().sum ());
      _cache.cache(_index[9], bld.ipmFexData().xpos());
      _cache.cache(_index[10],bld.ipmFexData().ypos());

      _cache.cache(_index[11], bld.ipimbData().channel0psVolts());
      _cache.cache(_index[12], bld.ipimbData().channel1psVolts());
      _cache.cache(_index[13], bld.ipimbData().channel2psVolts());
      _cache.cache(_index[14], bld.ipimbData().channel3psVolts());
    } break;
  default:
    break;
  }  
}

void SharedIpimbReader::_damaged  ()
{
  for(unsigned i=0; i<NChannels; i++)
    _cache.cache(_index[i], 0, true);
}

//  No Entry data
unsigned     SharedIpimbReader::nentries() const { return 0; }
const Entry* SharedIpimbReader::entry   (unsigned) const { return 0; }
void         SharedIpimbReader::rename  (const char* s)
{
  char buffer[64];
  strncpy(buffer,s,60);
  char* iptr = buffer+strlen(buffer);
  
  unsigned i=0;
  sprintf(iptr,":DATA[0]");  _rename_cache(_index[i],buffer); i++;
  sprintf(iptr,":DATA[1]");  _rename_cache(_index[i],buffer); i++;
  sprintf(iptr,":DATA[2]");  _rename_cache(_index[i],buffer); i++;
  sprintf(iptr,":DATA[3]");  _rename_cache(_index[i],buffer); i++;

  sprintf(iptr,":FEX:CH0");  _rename_cache(_index[i],buffer); i++;
  sprintf(iptr,":FEX:CH1");  _rename_cache(_index[i],buffer); i++;
  sprintf(iptr,":FEX:CH2");  _rename_cache(_index[i],buffer); i++;
  sprintf(iptr,":FEX:CH3");  _rename_cache(_index[i],buffer); i++;
  
  sprintf(iptr,":FEX:SUM");  _rename_cache(_index[i],buffer); i++;
  sprintf(iptr,":FEX:XPOS"); _rename_cache(_index[i],buffer); i++;
  sprintf(iptr,":FEX:YPOS"); _rename_cache(_index[i],buffer); i++;

  if (_index[i] != _index[0]) {
    sprintf(iptr,":PS:DATA[0]");  _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":PS:DATA[1]");  _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":PS:DATA[2]");  _rename_cache(_index[i],buffer); i++;
    sprintf(iptr,":PS:DATA[3]");  _rename_cache(_index[i],buffer); i++;
  }
}
