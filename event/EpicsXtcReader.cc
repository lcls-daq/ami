#include "EpicsXtcReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pdsdata/psddl/epics.ddl.h"

#include <stdio.h>

namespace Ami {
  class EpicsInfo : public Pds::Src {
  public:
    EpicsInfo() : Pds::Src(Pds::Level::Segment) { _phy=1; }
  };
};

using namespace Pds::Epics;

Ami::EpicsXtcReader::EpicsXtcReader(const Pds::Src& info, Ami::FeatureCache& f)  : 
  Ami::EventHandler(info,
               Pds::TypeId::Id_Epics,
               Pds::TypeId::Id_Epics),
  _cache(f)
{
}

Ami::EpicsXtcReader::~EpicsXtcReader()
{
}

void   Ami::EpicsXtcReader::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   Ami::EpicsXtcReader::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const EpicsPvHeader& pvData = 
    *reinterpret_cast<const EpicsPvHeader*>(payload);

  if (pvData.dbrType() >= DBR_CTRL_SHORT &&
      pvData.dbrType() <= DBR_CTRL_DOUBLE) {
    const EpicsPvCtrlHeader& ctrl = static_cast<const EpicsPvCtrlHeader&>(pvData);

    if (pvData.pvId() < 0) {
      printf("EpicsXtcReader found pv %s id %d.  Ignoring.\n",ctrl.pvName(),pvData.pvId());
      return;
    }

    if (pvData.pvId() >= MaxPvs) {
      printf("EpicsXtcReader found pv %s id %d > %d.  Ignoring.\n",ctrl.pvName(),pvData.pvId(),MaxPvs);
      return;
    }

    int index = -1;
    if (ctrl.numElements()>32) {
      printf("PV array %s[%d] too large. Ignored.\n",ctrl.pvName(),ctrl.numElements());
    }
    else if (ctrl.numElements()>1) {
      char buffer[64];
      strncpy(buffer,ctrl.pvName(),64);
      char* iptr = buffer+strlen(buffer);
      for(unsigned i=0; i<unsigned(ctrl.numElements()); i++) {
	sprintf(iptr,"[%d]",i);
	index = _cache.add(buffer);
      }
      index -= ctrl.numElements()-1;
    }
    else {
      index = _cache.add(ctrl.pvName());
    }

    if (ctrl.pvId() < MaxPvs) {
      _index[ctrl.pvId()] = index;
    }
    else
      printf("PV %s truncated from list\n",ctrl.pvName());
  }
  else {
    printf("EpicsXtcReader::_configure ignoring PV type %d\n",pvData.dbrType());
  }
}

#define CASETOVAL(timetype,pdstimetype,valtype) case timetype: {        \
    const pdstimetype& p = static_cast<const pdstimetype&>(pvData);     \
    const valtype* v = p.data().data();                                 \
    for(int i=0; i<pvData.numElements(); i++)				\
      _cache.cache(index++, *v++);					\
    break; }

void   Ami::EpicsXtcReader::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const EpicsPvHeader& pvData = *reinterpret_cast<const EpicsPvHeader*>(payload);

  if (pvData.pvId() <  0 || 
      pvData.pvId() >= MaxPvs)
    return;

  if (pvData.dbrType() < DBR_CTRL_STRING) {
    int index = _index[pvData.pvId()];
    if (index<0)
      return;
    switch(pvData.dbrType()) {
      CASETOVAL(DBR_TIME_SHORT ,EpicsPvTimeShort , int16_t )
      CASETOVAL(DBR_TIME_FLOAT ,EpicsPvTimeFloat , float   )
      CASETOVAL(DBR_TIME_ENUM  ,EpicsPvTimeEnum  , uint16_t)
      CASETOVAL(DBR_TIME_LONG  ,EpicsPvTimeLong  , int32_t )
      CASETOVAL(DBR_TIME_DOUBLE,EpicsPvTimeDouble, double  )
      break;
    }
  }
}

void   Ami::EpicsXtcReader::_damaged  ()
{
  for(unsigned i=0; i<MaxPvs; i++)
    if (_index[i]>=0)
      _cache.cache(_index[i], 0, true);
}

//  No Entry data
unsigned          Ami::EpicsXtcReader::nentries() const { return 0; }
const Ami::Entry* Ami::EpicsXtcReader::entry   (unsigned) const { return 0; }
void              Ami::EpicsXtcReader::reset   () 
{
  for(unsigned i=0; i<MaxPvs; i++)
    _index[i] = -1;
}
