#ifndef Ami_SyncAnalysis_hh
#define Ami_SyncAnalysis_hh

#include <stdio.h>
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "pdsdata/acqiris/ConfigV1.hh"
#include "pdsdata/xtc/DetInfo.hh"


#include "pdsdata/acqiris/DataDescV1.hh"
#include "pdsdata/princeton/ConfigV1.hh"
#include "pdsdata/pnCCD/ConfigV1.hh"

namespace Pds {
  class ClockTime;
  class Src;
  class TypeId;
};

namespace Ami {

  class SyncAnalysis {
  public:
    SyncAnalysis (const Pds::DetInfo& detInfo, Pds::TypeId::Type dataType,
	                 Pds::TypeId::Type configType, void* payload, const char* title);
    ~SyncAnalysis(); 
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
    void*    configPayload()                  { return _configPayload; }	
    void*    dataPayload()                    { return _dataPayload; }	 
    void     logDataPoint(double val, bool darkShot);

  private:
    Pds::TypeId::Type          _dataType;
    Pds::TypeId::Type          _configType;
    void*                      _configPayload;	 
    void*                      _dataPayload; 
  	
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
    bool                 _arrayBuiltFlag;
    bool                 _newEvent;
    double               _valMin;
    double               _valMax;
    double               _scalingFactor; 
    
  private:
    unsigned _litePayloadsize;
    unsigned _darkPayloadsize;
    unsigned long long* _litePayload;
    unsigned long long* _darkPayload;
 	
  };
};

#endif





