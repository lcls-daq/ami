#include "EpicsXtcReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pdsdata/psddl/epics.ddl.h"

#include <stdio.h>

static bool _use_alias     =false;
static bool _use_alias_only=false;

namespace Ami {
  class EpicsInfo : public Pds::Src {
  public:
    EpicsInfo() : Pds::Src(Pds::Level::Segment) { _phy=1; }
  };
};

using namespace Pds::Epics;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_Epics);
  types.push_back(Pds::TypeId::Id_EpicsConfig);
  return types;
}

Ami::EpicsXtcReader::EpicsXtcReader(const Pds::Src& info, Ami::FeatureCache& f)  : 
  Ami::EventHandlerF(info,
		     Pds::TypeId::Id_Epics,
		     config_type_list(),
		     f)
{
  reset();
  _indexpv   .resize(1024);
  _alias     .resize(1024);
  _indexalias.resize(1024);
}

Ami::EpicsXtcReader::~EpicsXtcReader()
{
}

void   Ami::EpicsXtcReader::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}
void   Ami::EpicsXtcReader::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  switch(id.id()) {
  case Pds::TypeId::Id_Epics:
    { const EpicsPvHeader& pvData = 
        *reinterpret_cast<const EpicsPvHeader*>(payload);

      if (pvData.dbrType() >= DBR_CTRL_SHORT &&
          pvData.dbrType() <= DBR_CTRL_DOUBLE) {
        const EpicsPvCtrlHeader& ctrl = static_cast<const EpicsPvCtrlHeader&>(pvData);

        if (pvData.pvId() < 0) {
          printf("EpicsXtcReader found pv %s id %d.  Ignoring.\n",ctrl.pvName(),pvData.pvId());
          return;
        }

        if (pvData.pvId() >= int(_indexpv.size())) {
          printf("EpicsXtcReader found pv %s id %d > %zu.  Ignoring.\n",ctrl.pvName(),pvData.pvId(),_indexpv.size());
          return;
        }

        int pvId  = ctrl.pvId();
        int index=-1, aindex=-1;
        bool has_alias = (_alias[pvId].size()>0);
        if (ctrl.numElements()>32) {
          printf("PV array %s[%d] too large. Ignored.\n",ctrl.pvName(),ctrl.numElements());
        }
        else {
          if (_use_alias && has_alias)
            aindex = (ctrl.numElements()>1) ?
              _add_array_to_cache(_alias[pvId].c_str(),
                                  ctrl.numElements()) :
              _add_to_cache(_alias[pvId].c_str());
          if (!_use_alias_only || !has_alias)
            index = (ctrl.numElements()>1) ?
              _add_array_to_cache(ctrl.pvName(),
                                  ctrl.numElements()) :
              _add_to_cache(ctrl.pvName());
        }
        _indexalias[pvId] = aindex;
        _indexpv   [pvId] = index;
      }
      else {
        if (pvData.dbrType() != DBR_CTRL_STRING) // avoid log spam due to string pvs
          printf("EpicsXtcReader::_configure ignoring PV type %d\n",pvData.dbrType());
      }
    } break;
  case Pds::TypeId::Id_EpicsConfig:
    { const ConfigV1& pvData = 
        *reinterpret_cast<const ConfigV1*>(payload);
      unsigned iNumPv = pvData.numPv();
      _indexpv   .resize(iNumPv);
      _alias     .resize(iNumPv);
      _indexalias.resize(iNumPv);

      for(unsigned i=0; i<iNumPv; i++) {
        const PvConfigV1& pv = pvData.getPvConfig()[i];
        _alias[pv.pvId()] = std::string(pv.description());
      }
    } break;
  default:
    break;
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
      pvData.pvId() >= int(_indexpv.size()))
    return;

  if (pvData.dbrType() < DBR_CTRL_STRING) {
    int index = _indexalias[pvData.pvId()];
    if (_use_alias && index>=0) {
      switch(pvData.dbrType()) {
        CASETOVAL(DBR_TIME_SHORT ,EpicsPvTimeShort , int16_t )
          CASETOVAL(DBR_TIME_FLOAT ,EpicsPvTimeFloat , float   )
          CASETOVAL(DBR_TIME_ENUM  ,EpicsPvTimeEnum  , uint16_t)
          CASETOVAL(DBR_TIME_LONG  ,EpicsPvTimeLong  , int32_t )
          CASETOVAL(DBR_TIME_DOUBLE,EpicsPvTimeDouble, double  )
          break;
      }
    }
    if (!(_use_alias_only && index>=0)) {
      index = _indexpv[pvData.pvId()];
      if (index>=0)
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
}

//  No Entry data
unsigned          Ami::EpicsXtcReader::nentries() const { return 0; }
const Ami::Entry* Ami::EpicsXtcReader::entry   (unsigned) const { return 0; }
void              Ami::EpicsXtcReader::rename(const char*) {}

void              Ami::EpicsXtcReader::use_alias(bool only)
{
  _use_alias     =true;
  _use_alias_only=only;
}
