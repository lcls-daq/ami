#include "Gsc16aiHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/gsc16ai.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

Gsc16aiHandler::Gsc16aiHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
		Pds::TypeId::Id_Gsc16aiData,
		Pds::TypeId::Id_Gsc16aiConfig,
		f)
{
}

Gsc16aiHandler::~Gsc16aiHandler()
{
}

void   Gsc16aiHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   Gsc16aiHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  char buffer[68];
  strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),59);
  char* cc = buffer+strlen(buffer);
  int ii;

  _config = *reinterpret_cast<const Pds::Gsc16ai::ConfigV1*>(payload);
  switch(_config.voltageRange()) {
    case Pds::Gsc16ai::ConfigV1::VoltageRange_10V:
      _voltsMin = -10.0;
      _voltsPerCount = 20.0 / 0xffffu;
      break;
    case Pds::Gsc16ai::ConfigV1::VoltageRange_5V:
      _voltsMin = -5.0;
      _voltsPerCount = 10.0 / 0xffffu;
      break;
    case Pds::Gsc16ai::ConfigV1::VoltageRange_2_5V:
      _voltsMin = -2.5;
      _voltsPerCount = 5.0 / 0xffffu;
      break;
    default:
      _voltsMin = _voltsPerCount = 0.0;
      fprintf(stderr, "Error: gsc16ai data voltage range %hd not recognized\n",
              _config.voltageRange());
      break;
  }

  for (ii = _config.firstChan(); ii <= _config.lastChan(); ii++) {
    sprintf(cc, ":Ch%02d", ii);
    if (ii == _config.firstChan()) {
      _add_to_cache(buffer);
    } else {
      (void) _add_to_cache(buffer);
    }
  }
}

void   Gsc16aiHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  int ii;
  const Pds::Gsc16ai::DataV1& d = *reinterpret_cast<const Pds::Gsc16ai::DataV1*>(payload);

  for (ii = 0; ii <= _config.lastChan() - _config.firstChan(); ii++) {
    if (_config.dataFormat() == Pds::Gsc16ai::ConfigV1::DataFormat_OffsetBinary) {
      // offset binary data format
      _cache.cache(_index+ii, _voltsMin + (_voltsPerCount * d.channelValue(_config)[ii]));
    } else {
      // two's complement data format
      _cache.cache(_index+ii, _voltsPerCount * (int16_t)d.channelValue(_config)[ii]);
    }
  }
}

//  No Entry data
unsigned     Gsc16aiHandler::nentries() const { return 0; }
const Entry* Gsc16aiHandler::entry   (unsigned) const { return 0; }

void   Gsc16aiHandler::rename(const char* s)
{
  if (_index<0) return;

  char buffer[68];
  strncpy(buffer,s,59);
  char* cc = buffer+strlen(buffer);
  unsigned index(_index);
  for (int ii = _config.firstChan(); ii <= _config.lastChan(); ii++,index++) {
    sprintf(cc, ":Ch%02d", ii);
    _rename_cache(index,buffer);
  }
}
