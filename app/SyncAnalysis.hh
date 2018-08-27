#ifndef Ami_SyncAnalysis_hh
#define Ami_SyncAnalysis_hh

#include <stdio.h>
#include <math.h>
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "pdsdata/psddl/acqiris.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/pnccd.ddl.h"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/opal1k.ddl.h"
#include "pdsdata/psddl/fccd.ddl.h"
#include "pdsdata/psddl/ipimb.ddl.h"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/psddl/bld.ddl.h"
#include "pds/config/AcqConfigType.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/PrincetonConfigType.hh"
#include "pds/config/PrincetonDataType.hh"
#include "pds/config/FliConfigType.hh"
#include "pds/config/FliDataType.hh"
#include "pds/config/AndorConfigType.hh"
#include "pds/config/AndorDataType.hh"
#include "pds/config/ZylaConfigType.hh"
#include "pds/config/ZylaDataType.hh"
#include "pds/config/JungfrauConfigType.hh"
#include "pds/config/JungfrauDataType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/config/FrameFccdConfigType.hh"
#include "pds/config/IpimbDataType.hh"
#include "pds/config/PimaxConfigType.hh"
#include "pds/config/PimaxDataType.hh"
#include "pds/config/PixisConfigType.hh"
#include "pds/config/PixisDataType.hh"
#include "pds/config/EpicsCamConfigType.hh"
#include "pds/config/EpicsCamDataType.hh"

typedef Pds::Camera::FrameV1 FrameType;

static Pds::TypeId _frameType(Pds::TypeId::Id_Frame,
            FrameType::Version);

namespace Pds {
  class ClockTime;
  class Src;
  class TypeId;
};


namespace Ami {

  template <class CONFIG, class DATA> class DataSpace;

  class SyncAnalysis {
  public:
    SyncAnalysis (const Pds::DetInfo& detInfo, Pds::TypeId::Type dataType,
                   Pds::TypeId::Type configType, void* payload, const char* title);
    virtual ~SyncAnalysis();
  public:
    Pds::DetInfo detInfo;
    void syncReset();
    void logConfigPayload    (void* payload);
    void logEventDataPayload (void* payload);
    void buildArray          (unsigned liteDataLength,unsigned darkDataLength);

  public:
    unsigned liteArrayLength()    const       { return _liteArrayLength; }
    unsigned darkArrayLength()    const       { return _darkArrayLength; }
    unsigned getLiteShotIndex()   const       { return _liteShotIndex; }
    unsigned getDarkShotIndex()   const       { return _darkShotIndex; }
    unsigned statLiteShotsFull()  const       { return _liteShotsFull; }
    unsigned statDarkShotsFull()  const       { return _darkShotsFull; }
    unsigned getOffByOneStatus()  const       { return _offByOneStatus; }
    double   getLiteShotVal()     const       { return _liteShotValue; }
    double   getDarkShotVal()     const       { return _darkShotValue; }
    double   getValMin()          const       { return _valMin; }
    double   getValMax()          const       { return _valMax; }
    double   getScalingFactor()   const       { return _scalingFactor; }
    double*  getLiteShotArray()   const       { return _liteShotArray; }
    double*  getDarkShotArray()   const       { return _darkShotArray; }
    bool     arrayBuiltFlag()     const       { return _arrayBuiltFlag; }
    bool     newEvent()           const       { return _newEvent; }
    const char* getTitle()        const       { return _title;}
    Pds::TypeId::Type getDataType()   const   { return _dataType; }
    Pds::TypeId::Type getConfigType() const   { return _configType; }
    void     setValMin(double val)            { _valMin = val; }
    void     setValMax(double val)            { _valMax = val; }
    void     setScalingFactor(double val)     { _scalingFactor = val; }
    void     setNewEventFlag(bool flag)       { _newEvent = flag; }
    void     setOffByOneStatus(unsigned val)  { _offByOneStatus = val; }
    void*    configPayload()                  { return _configPayload; }
    void*    dataPayload()                    { return _dataPayload; }
    void     logDataPoint(double val, bool darkShot);
    virtual void logDetPayload(void* payload) { printf("ERROR::virtual logDetPayload() \n");}
    virtual double processData() { printf("ERROR::virtual processData() \n"); return 0;}



  private:
    Pds::TypeId::Type      _dataType;
    Pds::TypeId::Type      _configType;
    void*                  _configPayload;
    void*                  _dataPayload;

  private:
    const char*          _title;
    double               _liteShotValue;
    double               _darkShotValue;
    double*              _liteShotArray;
    double*              _darkShotArray;
    unsigned             _liteShotIndex;
    unsigned             _darkShotIndex;
    unsigned             _liteShotsFull;
    unsigned             _darkShotsFull;
    unsigned             _liteArrayLength;
    unsigned             _darkArrayLength;
    unsigned             _offByOneStatus;
    bool                 _arrayBuiltFlag;
    bool                 _newEvent;
    double               _valMin;
    double               _valMax;
    double               _scalingFactor;


  private:
    unsigned             _litePayloadsize;
    unsigned             _darkPayloadsize;
    unsigned long long*  _litePayload;
    unsigned long long*  _darkPayload;

  };

  typedef Ami::DataSpace <Opal1kConfigType,   FrameType>                 opalDataSpace;
  typedef Ami::DataSpace <TM6740ConfigType,   FrameType>                 pulnixDataSpace;
  typedef Ami::DataSpace <IpimbConfigType,    IpimbDataType>             ipimbDataSpace;
  typedef Ami::DataSpace <AcqConfigType,      Pds::Acqiris::DataDescV1>  acqDataSpace;
  typedef Ami::DataSpace <pnCCDConfigType,    Pds::PNCCD::FrameV1>       pnccdDataSpace;
  typedef Ami::DataSpace <PrincetonConfigType,PrincetonDataType>         princetonDataSpace;
  typedef Ami::DataSpace <FliConfigType,      FliDataType>               fliDataSpace;
  typedef Ami::DataSpace <AndorConfigType,    AndorDataType>             andorDataSpace;
  typedef Ami::DataSpace <ZylaConfigType,     ZylaDataType>              zylaDataSpace;
  typedef Ami::DataSpace <JungfrauConfigType,    JungfrauDataType>       jungfrauDataSpace;
  typedef Ami::DataSpace <PimaxConfigType,    PimaxDataType>             pimaxDataSpace;
  typedef Ami::DataSpace <PixisConfigType,    PixisDataType>             pixisDataSpace;
  typedef Ami::DataSpace <Pds::FCCD::FccdConfigV2, FrameType>            fccdDataSpace;
  typedef Ami::DataSpace <Pds::Bld::BldDataEBeamV3,  Pds::Bld::BldDataEBeamV3>     eBeamDataSpace;
  typedef Ami::DataSpace <Pds::Bld::BldDataPhaseCavity,     Pds::Bld::BldDataPhaseCavity>      phaseCavityDataSpace;
  typedef Ami::DataSpace <Pds::Bld::BldDataFEEGasDetEnergy, Pds::Bld::BldDataFEEGasDetEnergy>  gasDetectorDataSpace;
  typedef Ami::DataSpace <Pds::Bld::BldDataGMDV2,     Pds::Bld::BldDataGMDV2>          gmdDataSpace;
  typedef Ami::DataSpace <EpicsCamConfigType, EpicsCamDataType>          epicsCamDataSpace;

  template <class CONFIG, class DATA>
  class DataSpace : public SyncAnalysis {
    public:
      DataSpace<CONFIG, DATA>(const Pds::DetInfo& detInfo, Pds::TypeId::Type dataType,Pds::TypeId::Type configType, void* payload, const char* title):
        SyncAnalysis(detInfo, dataType, configType, payload, title) {
        detConfig = *reinterpret_cast<CONFIG*>(payload);
        detConfigPtr = &detConfig;
      }
      double processData();
      void logDetPayload(void* payload) { detDataPtr = reinterpret_cast<DATA*>(payload); }

    public:
      CONFIG  detConfig;
      CONFIG* detConfigPtr;
      DATA*   detDataPtr;
    };

};

#endif





