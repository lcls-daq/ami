#include "EBeamReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>

using namespace Ami;

EBeamReader::EBeamReader(FeatureCache& f)  : 
  EventHandlerF(Pds::BldInfo(0,Pds::BldInfo::EBeam),
		Pds::TypeId::Id_EBeam,
		Pds::TypeId::Id_EBeam,
		f)
{
}

EBeamReader::~EBeamReader()
{
}

void   EBeamReader::_calibrate(Pds::TypeId id,
                               const void* payload, 
                               const Pds::ClockTime& t) 
{}

void   EBeamReader::_configure(Pds::TypeId id,
                               const void* payload, 
                               const Pds::ClockTime& t) 
{
  _add_to_cache("BLD:EBEAM:Q");
  _add_to_cache("BLD:EBEAM:L3E");
  _add_to_cache("BLD:EBEAM:LTUX");
  _add_to_cache("BLD:EBEAM:LTUY");
  _add_to_cache("BLD:EBEAM:LTUXP");
  int index = _add_to_cache("BLD:EBEAM:LTUYP");

  if (id.version()>=1) {
    index = _add_to_cache("BLD:EBEAM:PKCURRBC2");
    if (id.version()>=2) {
      _add_to_cache("BLD:EBEAM:PKCURRBC2");
      index = _add_to_cache("BLD:EBEAM:ENERGYBC2");
      if (id.version()>=3) {
        _add_to_cache("BLD:EBEAM:PKCURRBC1");
        index = _add_to_cache("BLD:EBEAM:ENERGYBC1");
        if (id.version()>=4) {
          _add_to_cache("BLD:EBEAM:UNDX");
          _add_to_cache("BLD:EBEAM:UNDY");
          _add_to_cache("BLD:EBEAM:UNDXP");
          index = _add_to_cache("BLD:EBEAM:UNDYP");
          if (id.version()>=5) {
            _add_to_cache("BLD:EBEAM:XTCAVAMPL");
            _add_to_cache("BLD:EBEAM:XTCAVPHASE");
            index = _add_to_cache("BLD:EBEAM:QDUMP");
            if (id.version()>=6) {
              _add_to_cache("BLD:EBEAM:PHOTENERGY");
              _add_to_cache("BLD:EBEAM:LTU250");
              index = _add_to_cache("BLD:EBEAM:LTU450");
            }
          }
        }
      }
    }
  }
}

void   EBeamReader::_event    (Pds::TypeId id,
                               const void* payload, 
                               const Pds::ClockTime& t)
{
}

void   EBeamReader::_event    (Pds::TypeId id,
                               const void* payload, 
                               const Pds::ClockTime& t,
                               Pds::Damage dmg)
{
#define TEST(val) {                                                  \
  if (bld.damageMask()&bld.Ebeam##val##Damage)                       \
    _cache.cache(index++,-1,true);                                   \
  else                                                               \
    _cache.cache(index++,bld.ebeam##val());                          \
  }

  /* LTU250 and LTU450 are shot-by-shot BPM measurements
     that are used to compute the photon energy value.  The 
     PhotonEnergy bit reflects problems with any of the 3 numbers. */

#define TESTLTU(val) {                                               \
  if (bld.damageMask()&bld.EbeamPhotonEnergyDamage)                       \
    _cache.cache(index++,-1,true);                                   \
  else                                                               \
    _cache.cache(index++,bld.ebeam##val());                          \
  }

#if 0
  if (dmg.value()) {
    printf("EBeamReader damage 0x%x [%x] [%x]\n",
           dmg.value(),
           dmg.value()&~Pds::Damage::UserDefined,
           *reinterpret_cast<const uint32_t*>(payload));
  }
#endif

  if (dmg.value() & ~(1<<Pds::Damage::UserDefined)) {
    _damaged();
    return;
  }

  if (_index>=0) {
    unsigned index = _index;

    switch(id.version()) {
    case 0:
      {
        const Pds::Bld::BldDataEBeamV0& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV0*>(payload);
        TEST(Charge);
        TEST(L3Energy);
        TEST(LTUPosX);
        TEST(LTUPosY);
        TEST(LTUAngX);
        TEST(LTUAngY);
        break;
      }
    case 1:
      {
        const Pds::Bld::BldDataEBeamV1& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV1*>(payload);
        TEST(Charge);
        TEST(L3Energy);
        TEST(LTUPosX);
        TEST(LTUPosY);
        TEST(LTUAngX);
        TEST(LTUAngY);
        TEST(PkCurrBC2);
        break;
      }
    case 2:
      {
        const Pds::Bld::BldDataEBeamV2& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV2*>(payload);
        TEST(Charge);
        TEST(L3Energy);
        TEST(LTUPosX);
        TEST(LTUPosY);
        TEST(LTUAngX);
        TEST(LTUAngY);
        TEST(PkCurrBC2);
        TEST(EnergyBC2);
        break;
      }
    case 3:
      {
        const Pds::Bld::BldDataEBeamV3& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV3*>(payload);
        TEST(Charge);
        TEST(L3Energy);
        TEST(LTUPosX);
        TEST(LTUPosY);
        TEST(LTUAngX);
        TEST(LTUAngY);
        TEST(PkCurrBC2);
        TEST(EnergyBC2);
        TEST(PkCurrBC1);
        TEST(EnergyBC1);
        break;
      }
    case 4:
      {
        const Pds::Bld::BldDataEBeamV4& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV4*>(payload);
        TEST(Charge);
        TEST(L3Energy);
        TEST(LTUPosX);
        TEST(LTUPosY);
        TEST(LTUAngX);
        TEST(LTUAngY);
        TEST(PkCurrBC2);
        TEST(EnergyBC2);
        TEST(PkCurrBC1);
        TEST(EnergyBC1);
        TEST(UndPosX);
        TEST(UndPosY);
        TEST(UndAngX);
        TEST(UndAngY);
        break;
      }
    case 5:
      {
        const Pds::Bld::BldDataEBeamV5& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV5*>(payload);
        TEST(Charge);
        TEST(L3Energy);
        TEST(LTUPosX);
        TEST(LTUPosY);
        TEST(LTUAngX);
        TEST(LTUAngY);
        TEST(PkCurrBC2);
        TEST(EnergyBC2);
        TEST(PkCurrBC1);
        TEST(EnergyBC1);
        TEST(UndPosX);
        TEST(UndPosY);
        TEST(UndAngX);
        TEST(UndAngY);
        TEST(XTCAVAmpl);
        TEST(XTCAVPhase);
        TEST(DumpCharge);
        break;
      }
    case 6:
    case 7:
      {
        const Pds::Bld::BldDataEBeamV6& bld = 
          *reinterpret_cast<const Pds::Bld::BldDataEBeamV6*>(payload);
        TEST(Charge);
        TEST(L3Energy);
        TEST(LTUPosX);
        TEST(LTUPosY);
        TEST(LTUAngX);
        TEST(LTUAngY);
        TEST(PkCurrBC2);
        TEST(EnergyBC2);
        TEST(PkCurrBC1);
        TEST(EnergyBC1);
        TEST(UndPosX);
        TEST(UndPosY);
        TEST(UndAngX);
        TEST(UndAngY);
        TEST(XTCAVAmpl);
        TEST(XTCAVPhase);
        TEST(DumpCharge);
        TEST(PhotonEnergy);
        TESTLTU(LTU250);
        TESTLTU(LTU450);
        break;
      }
    default:
      break;
    }
  }

#undef TEST
}

//  No Entry data
unsigned     EBeamReader::nentries() const { return 0; }
const Entry* EBeamReader::entry   (unsigned) const { return 0; }
void         EBeamReader::rename(const char*) {}
