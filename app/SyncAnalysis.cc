#include "ami/app/SyncAnalysis.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

SyncAnalysis::SyncAnalysis(const Pds::DetInfo& detInfo, Pds::TypeId::Type dataType,
	                          Pds::TypeId::Type configType, void* payload, const char* title):
  detInfo(detInfo),
  _dataType(dataType),
  _configType(configType),
  _configPayload(payload),
  _title(title),
  _liteShotsFull(0),
  _darkShotsFull(0),
  _arrayBuiltFlag(false),
  _valMin(0.0),
  _valMax(5000.0),
  _scalingFactor(0.02)
{
  _configPayload = ((void*)(new unsigned long long[200]));

} 

SyncAnalysis::~SyncAnalysis()
{
  if (_litePayload)
    delete [] _litePayload; 
  if (_darkPayload)
    delete [] _darkPayload; 
};

void SyncAnalysis::syncReset() 
{
  _liteShotIndex = 0;
  _darkShotIndex = 0;
  _liteShotValue = 0;
  _darkShotValue = 0; 
  _darkShotsFull = 0;
  _liteShotsFull = 0;
  memset(_litePayload, 0, _litePayloadsize);   
  memset(_darkPayload, 0, _darkPayloadsize); 

}


void SyncAnalysis::logConfigPayload    (void* payload)
{
   _configPayload = payload;	

}

void SyncAnalysis::logEventDataPayload (void* payload)
{
   _dataPayload = payload;	
   _newEvent = true;
}


void SyncAnalysis::buildArray(unsigned liteDataLength,unsigned darkDataLength)
{
  _litePayloadsize = sizeof(unsigned long long) + (sizeof(double)*liteDataLength);
  _litePayload = new unsigned long long[(_litePayloadsize>>3)+1];
  memset(_litePayload, 0, _litePayloadsize);  
  _liteShotArray = static_cast<double*> ((void*)(_litePayload+1));

  _darkPayloadsize = sizeof(unsigned long long) + (sizeof(double)*darkDataLength);
  _darkPayload = new unsigned long long[(_darkPayloadsize>>3)+1];
  memset(_darkPayload, 0, _darkPayloadsize);  
  _darkShotArray = static_cast<double*> ((void*)(_darkPayload+1));

  _liteArrayLength = liteDataLength;
  _darkArrayLength = darkDataLength;
  _arrayBuiltFlag  = true;
  _liteShotIndex   = 0;
  _darkShotIndex   = 0;
  _liteShotValue   = 0;
  _darkShotValue   = 0;
  _darkShotsFull   = 0;
  _liteShotsFull   = 0;  

}


void SyncAnalysis::logDataPoint(double val, bool darkShot)
{
  if (darkShot) {
    _darkShotValue = val;
    *(_darkShotArray+ _darkShotIndex) = val;
    if(_darkShotIndex == (_darkArrayLength-1)) {
      _darkShotIndex = 0;
      _darkShotsFull = 1;
    } else _darkShotIndex++;        
  } else {
    _liteShotValue = val;
    *(_liteShotArray+ _liteShotIndex) = val;
    if(_liteShotIndex == (_liteArrayLength-1)) {
      _liteShotIndex = 0;
      _liteShotsFull = 1;
    } else _liteShotIndex++;
  }

  if (darkShot) 
    printf("%s - val = %f Gval = %f Dval = %f  diff = %f darkIndex = %u\n",_title,val,_liteShotValue,_darkShotValue,(_liteShotValue-_darkShotValue),_darkShotIndex); 

}




