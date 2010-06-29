#include "ami/app/SummaryAnalysis.hh"
#include "ami/app/SyncAnalysis.hh"

#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"

#include "pdsdata/acqiris/DataDescV1.hh"
#include "pdsdata/xtc/ClockTime.hh" 

#include "pdsdata/camera/FrameV1.hh"
#include "pdsdata/camera/FrameFexConfigV1.hh"
#include "pdsdata/opal1k/ConfigV1.hh"

#include "pdsdata/fccd/FccdConfigV2.hh"

#include "pdsdata/princeton/FrameV1.hh"
#include "pdsdata/princeton/ConfigV1.hh"

#include "pdsdata/pnCCD/FrameV1.hh"
#include "pdsdata/pnCCD/ConfigV1.hh"

#include "pdsdata/ipimb/DataV1.hh"
#include "pdsdata/ipimb/ConfigV1.hh"


#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/bld/bldData.hh"
#include <stdio.h>
#include <math.h>


#define DARK_SHOT_EVENT_CODE    162
#define DATA_RECORD_POINTS      10000,1000       //LiteShots, DarkShots points 
#define DISPLAY_N_POINTS        8000
#define REFILL_AFTER_POINTS     1000 
#define MIN_MAX_MARGIN_PERCENT  10
#define NO_OF_BINS              500
#define X_MIN                   0
#define X_MAX                   100
#define AUTO_DETECTION_ENB      false

using namespace Ami;

static EntryScalar* _seconds = 0;
static EntryScalar* _minutes = 0;  
static Cds* _cds = 0;
static Pds::ClockTime _clk;


SummaryAnalysis::SummaryAnalysis():_summaryEntries(0),_analyzeCount(0),_notRefilledCount(0),_darkShot(false) { }
SummaryAnalysis::~SummaryAnalysis() 
{
  if (!_summaryEntryEList.empty())
    _summaryEntryEList.clear();

  if (!_syncAnalysisPList.empty())
    _syncAnalysisPList.clear();  
}

void SummaryAnalysis::reset () 
{ 
  printf("SummaryAnalysis::reset \n");
  if (!_syncAnalysisPList.empty())
    _syncAnalysisPList.clear();  
  _summaryEntries = 0;  

}

void SummaryAnalysis::clock (const Pds::ClockTime& clk) 
{ 
  //printf("SummaryAnalysis::clk \n"); 
  _clk=clk;
}

void SummaryAnalysis::configure ( const Pds::Src& src, const Pds::TypeId& type, void* payload)
{
  printf("SummaryAnalysis::Configure  %s \n", Pds::TypeId::name(type.id()));
  SyncAnalysis* h = 0;  
  const DetInfo& detInfo = reinterpret_cast<const Pds::DetInfo&>(src); 
  /*if (!(_summaryEntries == 0)) {
    for(PList::iterator it = _syncAnalysisPList.begin(); it != _syncAnalysisPList.end(); it++) {
      SyncAnalysis* syncPtr = *it;
      if ((detInfo.detector() == syncPtr->detInfo.detector()) &&
          (detInfo.detId()    == syncPtr->detInfo.detId())    &&
          (detInfo.device()   == syncPtr->detInfo.device())   &&
          (detInfo.devId()    == syncPtr->detInfo.devId()) ) {
        syncPtr->logConfigPayload(payload);
        return;
      }
    }
  }
  */ 

  switch(type.id())  {
    case Pds::TypeId::Id_AcqConfig:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_AcqWaveform,     Pds::TypeId::Id_AcqConfig,       payload, "Acqiris");
      memcpy(h->configPayload(), payload, sizeof(Pds::Acqiris::ConfigV1) );
      break;
    case Pds::TypeId::Id_Opal1kConfig:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_Frame,           Pds::TypeId::Id_Opal1kConfig,    payload, "Opal");
      break; 
    case Pds::TypeId::Id_FccdConfig:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_Frame,           Pds::TypeId::Id_FccdConfig,      payload, "Fccd");
      break; 
    case Pds::TypeId::Id_TM6740Config:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_Frame,           Pds::TypeId::Id_TM6740Config,    payload, "Pulnix");
      break; 
    case Pds::TypeId::Id_PrincetonConfig:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_PrincetonFrame,  Pds::TypeId::Id_PrincetonConfig, payload, "Princeton");
      memcpy(h->configPayload(), payload, sizeof(Pds::Princeton::ConfigV1) );
      break;
    case Pds::TypeId::Id_pnCCDconfig:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_pnCCDframe,      Pds::TypeId::Id_pnCCDconfig,     payload, "PnCCD");
      memcpy(h->configPayload(), payload, sizeof(Pds::PNCCD::ConfigV1) );
      break;   
    case Pds::TypeId::Id_IpimbConfig:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_IpimbData,       Pds::TypeId::Id_IpimbConfig,     payload, "IPIMB");  
      break;
    case Pds::TypeId::Id_FEEGasDetEnergy:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_FEEGasDetEnergy, Pds::TypeId::Id_FEEGasDetEnergy, payload, "GasDetector");
      break; 
    case Pds::TypeId::Id_EBeam:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_EBeam,           Pds::TypeId::Id_EBeam,           payload, "EBeam");       
      break;
    case Pds::TypeId::Id_PhaseCavity:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_PhaseCavity,     Pds::TypeId::Id_PhaseCavity,     payload, "PhaseCavity"); 
      break;

   	default: break;
  }
  if (!h)
    { }//printf("SummaryAnalysis::not supported configType: %s \n", Pds::TypeId::name(type.id()));
  else {
    //printf("SummaryAnalysis::received configType: %s \n", Pds::TypeId::name(type.id()));
    insert(h);
    printf("%d th Summary Entry made: %s/%d/%s/%d \n",_summaryEntries, detInfo.name(detInfo.detector()),
                                      detInfo.detId(), detInfo.name(detInfo.device()),detInfo.devId());  
	   _summaryEntries++;
    //h->logConfigPayload(payload);
  }

}
  

void SummaryAnalysis::event (const Pds::Src& src, const Pds::TypeId& type, void* payload)
{

  const DetInfo& detInfo = reinterpret_cast<const Pds::DetInfo&>(src); 
  if (type.id() == Pds::TypeId::Id_EvrData) 
		  _evrEventData = const_cast<Pds::EvrData::DataV3*>(reinterpret_cast<const Pds::EvrData::DataV3*>(payload));
  else if((type.id() == Pds::TypeId::Id_AcqWaveform)     || (type.id() == Pds::TypeId::Id_Frame) || 
          (type.id() == Pds::TypeId::Id_FEEGasDetEnergy) || (type.id() == Pds::TypeId::Id_EBeam) ||
          (type.id() == Pds::TypeId::Id_PhaseCavity)     || (type.id() == Pds::TypeId::Id_PrincetonFrame) ||
          (type.id() == Pds::TypeId::Id_pnCCDframe)      || (type.id() == Pds::TypeId::Id_IpimbData)  ) {
    if (_syncAnalysisPList.empty()) 
      printf("**** ERROR:: event() called while SyncEntry List is Empty \n");
    else {
      bool entryFoundFlag = false;   
      for(PList::iterator it = _syncAnalysisPList.begin(); it != _syncAnalysisPList.end(); it++) {
        SyncAnalysis* syncPtr = *it;
        if ((detInfo.detector() == syncPtr->detInfo.detector()) &&
            (detInfo.detId()    == syncPtr->detInfo.detId())    &&
            (detInfo.device()   == syncPtr->detInfo.device())   &&
            (detInfo.devId()    == syncPtr->detInfo.devId()) ) {
          syncPtr->logEventDataPayload(payload);
          entryFoundFlag = true;
          break;        
        }
      }
      if(!entryFoundFlag)
        printf("**** ERROR:: event() called & no Entry Present in List for given detector \n");
    }  
  }

}

void SummaryAnalysis::clear () 
{
  printf("SummaryAnalysis::clear\n");
  if (_seconds) 
    _cds->remove(_seconds);     
  if(_minutes)
    _cds->remove(_minutes);

  for(EList::iterator itSummary = _summaryEntryEList.begin(); itSummary != _summaryEntryEList.end(); itSummary++) {
    EntryTH1F* summaryEntry = *itSummary;
    if(summaryEntry)
      _cds->remove(summaryEntry);
  }
  if (!_summaryEntryEList.empty())
    _summaryEntryEList.clear();

   _cds = 0;

}


void SummaryAnalysis::create   (Cds& cds)
{
  printf("SummaryAnalysis::create\n");
  _notRefilledCount = 0;
  _analyzeCount = 0;
  _seconds = new EntryScalar(DescScalar("Seconds#Time","Seconds"));
  _minutes = new EntryScalar(DescScalar("Minutes#Time","Minutes"));
  
  _cds = &cds; 
  cds.add(_seconds);
  cds.add(_minutes);
  
  EntryTH1F* summaryEntry = 0;
  char displayTitle[50];  
  for(PList::iterator it = _syncAnalysisPList.begin(); it != _syncAnalysisPList.end(); it++) {
    SyncAnalysis* syncPtr = *it;
    if (syncPtr->arrayBuiltFlag())
      syncPtr->syncReset();                     // reset stored Data Buffers  
    else 
      syncPtr->buildArray(DATA_RECORD_POINTS);  // build Data Buffers 

    Pds::TypeId::Type  dataType = syncPtr->getDataType();
    // Lite Data Plots Entry
    if( (dataType == Pds::TypeId::Id_FEEGasDetEnergy) || (dataType == Pds::TypeId::Id_EBeam) || (dataType == Pds::TypeId::Id_PhaseCavity) )
      sprintf(displayTitle,"LightShotsSummary#%s",syncPtr->getTitle());
    else
      sprintf(displayTitle,"LightShotsSummary#%s/%d/%s/%d",syncPtr->detInfo.name(syncPtr->detInfo.detector()), syncPtr->detInfo.detId(),
                          syncPtr->detInfo.name(syncPtr->detInfo.device()),syncPtr->detInfo.devId());  
    summaryEntry = new EntryTH1F(syncPtr->detInfo, 0, displayTitle,"Range","Value");
    insertEntry(summaryEntry);
    summaryEntry->params(NO_OF_BINS, X_MIN, X_MAX); 
    cds.add(summaryEntry); 

    //Dark Data Plots Entry
    if( (dataType == Pds::TypeId::Id_FEEGasDetEnergy) || (dataType == Pds::TypeId::Id_EBeam) || (dataType == Pds::TypeId::Id_PhaseCavity) )
      sprintf(displayTitle,"DarkShotsSummary#%s",syncPtr->getTitle());
    else
      sprintf(displayTitle,"DarkShotsSummary#%s/%d/%s/%d",syncPtr->detInfo.name(syncPtr->detInfo.detector()), syncPtr->detInfo.detId(),
                          syncPtr->detInfo.name(syncPtr->detInfo.device()),syncPtr->detInfo.devId());  
    summaryEntry = new EntryTH1F(syncPtr->detInfo, 0, displayTitle,"Range","Value");
    insertEntry(summaryEntry);
    summaryEntry->params(NO_OF_BINS, X_MIN, X_MAX); 
    cds.add(summaryEntry); 

  }


}

void SummaryAnalysis::analyze  () 
{
  //printf("SummaryAnalysis::analyze \n");
  if (_cds) {

    // EVR Data Read for darkShot check
    for(unsigned i=0;i < _evrEventData->numFifoEvents(); i++) {
      if ( _evrEventData->fifoEvent(i).EventCode == DARK_SHOT_EVENT_CODE) { 
        _darkShot = true;
   	    printf("Dark Shot Found \n");
        break;
      } else  _darkShot = false;
    }

    //Process all detector's data
    for(PList::iterator it = _syncAnalysisPList.begin(); it != _syncAnalysisPList.end(); it++) {
      SyncAnalysis* syncPtr = *it;
        
      switch(syncPtr->getDataType()) {

        case Pds::TypeId::Id_AcqWaveform : 
          processAcqirisData(syncPtr);
          break;	 
        case Pds::TypeId::Id_Frame : 
          if (syncPtr->getConfigType() == Pds::TypeId::Id_Opal1kConfig)
            processOpalData(syncPtr);
          else if (syncPtr->getConfigType() == Pds::TypeId::Id_FccdConfig)
            processFccdData(syncPtr);
          else if (syncPtr->getConfigType() == Pds::TypeId::Id_TM6740Config)
            processPulnixData(syncPtr);
          else
            printf("**** ERROR:: Unsupported Detector with Id_frame dataType \n");  
          break;	
        case Pds::TypeId::Id_PrincetonFrame :  
          processPrincetonData(syncPtr);
          break;	
        case Pds::TypeId::Id_pnCCDframe :
          processPnccdData(syncPtr);
          break;	
        case Pds::TypeId::Id_IpimbData :
          processIpimbData(syncPtr);
          break;	
        case Pds::TypeId::Id_FEEGasDetEnergy : 
          processGasDetectorData(syncPtr);
          break;	
        case Pds::TypeId::Id_EBeam : 
          processEBeamData(syncPtr);
          break;	
        case Pds::TypeId::Id_PhaseCavity : 
          processPhaseCavityData(syncPtr);
          break;	  
	  
        default: 
          printf(" ERROR :: Processing Invalid Type data in analyze()\n");	
	         break;

      }
    }

    //Update Plot Data 
    PList::iterator it = _syncAnalysisPList.begin();
    for(EList::iterator itSummary = _summaryEntryEList.begin(); itSummary != _summaryEntryEList.end(); itSummary++, it++) {
      SyncAnalysis* syncPtr = *it;
      EntryTH1F* summaryEntry = *itSummary;
      itSummary++;
      if (_darkShot) 
        refillSummaryPlot(syncPtr, summaryEntry, *itSummary, DISPLAY_N_POINTS, ForceRefill) ; 
      else if(_notRefilledCount >= DISPLAY_N_POINTS)
        refillSummaryPlot(syncPtr, summaryEntry, *itSummary, DISPLAY_N_POINTS, ForceRefill) ;      
      else {
        if((_analyzeCount < 100) && ((_analyzeCount % 5)==0) )
          refillSummaryPlot(syncPtr, summaryEntry, *itSummary, DISPLAY_N_POINTS, ValidateRefill) ;
        else if((_analyzeCount % REFILL_AFTER_POINTS) == 0)
          refillSummaryPlot(syncPtr, summaryEntry, *itSummary, DISPLAY_N_POINTS, ValidateRefill) ;
        else
          summaryEntry->addcontent(1.0,fabs( (syncPtr->getLiteShotVal()-syncPtr->getValMin()) * syncPtr->getScalingFactor()));
      }  
    }

    if(_notRefilledCount >= DISPLAY_N_POINTS)
      _notRefilledCount = 1; 
    else
      _notRefilledCount++;
    _analyzeCount++;
    if ((_analyzeCount%1000 )== 0)
      printf("AnaEvents = %u noRefiilCnt = %u \n",_analyzeCount, _notRefilledCount); 


    /** Test Cases **/
    _seconds->valid(_clk);
    _minutes->valid(_clk);
    _seconds->addcontent(_clk.seconds()%60); 
		  _minutes->addcontent((_clk.seconds()/60)%60);

	
  }
}


void SummaryAnalysis::processAcqirisData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::Acqiris::ConfigV1* acqConfig = reinterpret_cast<const Pds::Acqiris::ConfigV1*>(syncPtr->configPayload()); 
    Pds::Acqiris::DataDescV1* acqData = const_cast<Pds::Acqiris::DataDescV1*>(reinterpret_cast<const Pds::Acqiris::DataDescV1*>(syncPtr->dataPayload()));
	   const Pds::Acqiris::HorizV1& h = acqConfig->horiz();
    unsigned n = acqConfig->nbrChannels();
    double val = 0;  
    for (unsigned i=0;i<n;i++) {	
      const int16_t* data = acqData->waveform(h);
      data += acqData->indexFirstPoint();
      float slope = acqConfig->vert(i).slope();
      float offset = acqConfig->vert(i).offset();
      unsigned nbrSamples = h.nbrSamples();
      for (unsigned j=0;j<nbrSamples;j++) {
        int16_t swap = (data[j]&0xff<<8) | (data[j]&0xff00>>8);
        val = val + fabs(swap*slope-offset);      //fabs value for area under curve 
      }
      acqData = acqData->nextChannel(h);	
    }
    syncPtr->logDataPoint(val, _darkShot);
    syncPtr->setNewEventFlag(false);
  } 

}

void SummaryAnalysis::processOpalData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::Camera::FrameV1& opalFrame = *reinterpret_cast<const Pds::Camera::FrameV1*>(syncPtr->dataPayload());
    const uint16_t* dataArray = reinterpret_cast<const uint16_t*>(opalFrame.data());  
    unsigned totalPixels = opalFrame.width()*opalFrame.height();
    unsigned offset = opalFrame.offset();
    unsigned val = 0; 
    for (unsigned i = 0 ; i<totalPixels ; i++)  { 
      if (*(dataArray+i) > offset)
        val = val + (*(dataArray+i) );             // opal data with Pedestal/Offset adjustment 
      //val = val + ((*(dataArray+i) ) >>1);       // opal data divide by 2 and then add all pixels
    }
    syncPtr->logDataPoint((double)val, _darkShot); 
    syncPtr->setNewEventFlag(false); 
  }
}

void SummaryAnalysis::processFccdData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    //const Pds::FCCD::FccdConfigV2& cfccd = *reinterpret_cast<const Pds::FCCD::FccdConfigV2*>(syncPtr->configPayload()); 
    const Pds::Camera::FrameV1& fccdFrame = *reinterpret_cast<const Pds::Camera::FrameV1*>(syncPtr->dataPayload());
    const uint16_t* dataArray = reinterpret_cast<const uint16_t*>(fccdFrame.data());  
    unsigned totalPixels = fccdFrame.width()*fccdFrame.height();
    unsigned offset = fccdFrame.offset();
    unsigned val = 0; 
    for (unsigned i = 0 ; i<totalPixels ; i++)  { 
      if (*(dataArray+i) > offset)
        val = val + (*(dataArray+i) );            // FCCD data with Pedestal/Offset adjustment 
      //val = val + ((*(dataArray+i) ) >>1);      // FCCD data divide by 2 and then add all pixels
    }
    syncPtr->logDataPoint((double)val, _darkShot); 
    syncPtr->setNewEventFlag(false);
    //printf("FrData: Width = %u Height= %u depth = %u offset = %u dep_bytes= %u data_size = %u pixel_50.30 = %u val = %u\n",
    //        fccdFrame.width(), fccdFrame.height(),fccdFrame.depth(),fccdFrame.offset(),fccdFrame.depth_bytes(),fccdFrame.data_size(),*fccdFrame.pixel(50,30) , val ); 
    //printf("FrData: DWidth = %u DHeight= %u  CWidth = %u CHeight= %u \n", fccdFrame.width(), fccdFrame.height(),cfccd.width(), cfccd.height() );

  }
}

void SummaryAnalysis::processPulnixData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::Camera::FrameV1& pulnixFrame = *reinterpret_cast<const Pds::Camera::FrameV1*>(syncPtr->dataPayload());
    const uint16_t* dataArray = reinterpret_cast<const uint16_t*>(pulnixFrame.data());  
    unsigned totalPixels = pulnixFrame.width()*pulnixFrame.height();
    unsigned offset = pulnixFrame.offset();
    unsigned val = 0; 
    for (unsigned i = 0 ; i<totalPixels ; i++)  { 
      if (*(dataArray+i) > offset)
        val = val + (*(dataArray+i) );             // Pulnix data with Pedestal/Offset adjustment 
      //val = val + ((*(dataArray+i) ) >>1);       // Pulnix data divide by 2 and then add all pixels
    }
    syncPtr->logDataPoint((double)val, _darkShot); 
    syncPtr->setNewEventFlag(false);
  }
}


void SummaryAnalysis::processPrincetonData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::Princeton::ConfigV1& princetonConfig = *reinterpret_cast<const Pds::Princeton::ConfigV1*>(syncPtr->configPayload()); 
    const Pds::Princeton::FrameV1&  princetonFrame   = *reinterpret_cast<const Pds::Princeton::FrameV1*>(syncPtr->dataPayload());
    const uint16_t* dataArray = reinterpret_cast<const uint16_t*>(princetonFrame.data());
    unsigned totalPixels = princetonConfig.width()*princetonConfig.height();

    unsigned val = 0;   
    for (unsigned i = 0 ; i<totalPixels ; i++) 
      val = val + ((*(dataArray+i) ) >>2);       // Princeton data divide by 4 and then add all pixels ??
    
    syncPtr->logDataPoint((double)val, _darkShot);
    syncPtr->setNewEventFlag(false); 
    //printf(" 2 FrData: Width = %u Height= %u pixel_50.30 = %u val = %u\n", princetonConfig.width(), princetonConfig.height(), *(dataArray+ ( princetonConfig.width() *30 + 50) ), val ); 

  }
 
}

void SummaryAnalysis::processPnccdData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::PNCCD::ConfigV1& pnccdConfig  = *reinterpret_cast<const Pds::PNCCD::ConfigV1*>(syncPtr->configPayload()); 
    const Pds::PNCCD::FrameV1*  pnccdFrame   = reinterpret_cast<const Pds::PNCCD::FrameV1*>(syncPtr->dataPayload());
    const uint16_t* dataArray = reinterpret_cast<const uint16_t*>(pnccdFrame->data());

    unsigned val = 0;
    for (unsigned j=0; j<4 ; j++ ) {                // for all 4 quadrants in sequence UpL+LoL+LoR+UpR
      unsigned dataSize = pnccdFrame->sizeofData(pnccdConfig);
      for(unsigned i= 0; i< dataSize ; i++) {
        //val = val + ((*(dataArray+i) ) >>1);        // PnCCD data divide by 2 and then add all pixels
        val = val + ( ((*(dataArray+i) ) & 0x3fff) >>1 ); 
      }
      pnccdFrame = pnccdFrame->next(pnccdConfig);
      dataArray = reinterpret_cast<const uint16_t*>(pnccdFrame->data());
    }
  
    syncPtr->logDataPoint((double)val, _darkShot);
    syncPtr->setNewEventFlag(false); 
  } 
}


void SummaryAnalysis::processIpimbData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::Ipimb::DataV1& ipimbData = *reinterpret_cast<const Pds::Ipimb::DataV1*>(syncPtr->dataPayload());
    //ipimb all 4 channels voltage abs() addition  -- Do we need average ??
    double val =  fabs(ipimbData.channel0Volts()) + fabs(ipimbData.channel1Volts()) + fabs(ipimbData.channel2Volts()) + fabs(ipimbData.channel3Volts()) ; 
    syncPtr->logDataPoint(val, _darkShot);
    syncPtr->setNewEventFlag(false); 
  } 
}


void SummaryAnalysis::processGasDetectorData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::BldDataFEEGasDetEnergy& gasDetectorData = *reinterpret_cast<const Pds::BldDataFEEGasDetEnergy*>(syncPtr->dataPayload());
    //gas detector abs() adddition of 4 energy sensors  -- Do we need average ??
    double val = fabs(gasDetectorData.f_11_ENRC) + fabs(gasDetectorData.f_12_ENRC) + fabs(gasDetectorData.f_21_ENRC) + fabs(gasDetectorData.f_22_ENRC); 
    //val = val/4.0;
    syncPtr->logDataPoint(val, _darkShot);
    syncPtr->setNewEventFlag(false); 
  } 
}

void SummaryAnalysis::processEBeamData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::BldDataEBeam& EBeamData = *reinterpret_cast<const Pds::BldDataEBeam*>(syncPtr->dataPayload());
    double val = fabs(EBeamData.fEbeamCharge); 
    syncPtr->logDataPoint(val, _darkShot);
    syncPtr->setNewEventFlag(false); 
  } 
}

void SummaryAnalysis::processPhaseCavityData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::BldDataPhaseCavity& phaseCavityData = *reinterpret_cast<const Pds::BldDataPhaseCavity*>(syncPtr->dataPayload());
    double val = fabs(phaseCavityData.fCharge1 + phaseCavityData.fCharge2)/2.0; 
    syncPtr->logDataPoint(val, _darkShot);
    syncPtr->setNewEventFlag(false); 
  } 
}


static SummaryAnalysis* _instance = 0;

SummaryAnalysis& SummaryAnalysis::instance() 
{
  if (!_instance)
    _instance = new SummaryAnalysis;
  return *_instance; 
}



void SummaryAnalysis::refillSummaryPlot(SyncAnalysis* syncPtr, EntryTH1F* summaryLiteEntry, EntryTH1F* summaryDarkEntry, unsigned points, unsigned refillType)
{

  unsigned dataArrayLength = syncPtr->liteArrayLength(); 
  double*  dataArray   = syncPtr->getLiteShotArray();
  unsigned statShotsFull = syncPtr->statLiteShotsFull();
  unsigned lookUpIndexHigh = syncPtr->getLiteShotIndex(); 
  unsigned lookUpIndexLow  = 0;

  double liteMinVal = 0;
  double liteMaxVal = 0;
  double darkMinVal = 0;
  double darkMaxVal = 0;
  unsigned liteLookUpIndexHigh = syncPtr->getLiteShotIndex(); 
  unsigned darkLookUpIndexHigh = syncPtr->getDarkShotIndex(); 
  unsigned liteLookUpIndexLow  = 0;
  unsigned darkLookUpIndexLow  = 0;
  double minVal = 0;
  double maxVal = 0;
  unsigned i=0;

  for(unsigned j=0; j<2 ; j++) {
    if (j == 1) {
      dataArrayLength = syncPtr->darkArrayLength(); 
      dataArray   = syncPtr->getDarkShotArray();
      statShotsFull = syncPtr->statDarkShotsFull(); 
      lookUpIndexHigh = syncPtr->getDarkShotIndex(); 
      lookUpIndexLow  = 0; 
    
    }   

    //Find Lower and Upper Look-Up Index
    if(lookUpIndexHigh >= points)
      lookUpIndexLow = lookUpIndexHigh - points;
    else if (statShotsFull == 1)
      lookUpIndexLow = dataArrayLength - (points-lookUpIndexHigh);

    //Find Min & Max Val for current Slot
    minVal = *(dataArray + lookUpIndexLow);
    maxVal = *(dataArray + lookUpIndexLow);
  
    if( lookUpIndexHigh < lookUpIndexLow) {
      for (i=lookUpIndexLow; i < dataArrayLength; i++) {
        if (minVal > *(dataArray+i)) minVal = *(dataArray+i);
        if (maxVal < *(dataArray+i)) maxVal = *(dataArray+i);
      }
      for (i=0; i < lookUpIndexHigh; i++) {
        if (minVal > *(dataArray+i)) minVal = *(dataArray+i);
        if (maxVal < *(dataArray+i)) maxVal = *(dataArray+i);
      }
    }
    else {
      for (i=lookUpIndexLow; i <lookUpIndexHigh; i++) {
        if (minVal > *(dataArray+i)) minVal = *(dataArray+i);
        if (maxVal < *(dataArray+i)) maxVal = *(dataArray+i);
      }
    }


    if (j == 1) {
      darkMinVal          = minVal;
      darkMaxVal          = maxVal; 
      darkLookUpIndexHigh = lookUpIndexHigh;    
      darkLookUpIndexLow  = lookUpIndexLow;
    } else {
      liteMinVal          = minVal;
      liteMaxVal          = maxVal; 
      liteLookUpIndexHigh = lookUpIndexHigh;    
      liteLookUpIndexLow  = lookUpIndexLow;
    } 
  }


  // consider darkMin & darkMax only if dark shot is present
  if ( (darkLookUpIndexHigh > 0)  || (syncPtr->statDarkShotsFull() == 1) )  {
    maxVal = (liteMaxVal >= darkMaxVal) ? liteMaxVal : darkMaxVal;
    minVal = (liteMinVal <= darkMinVal) ? liteMinVal : darkMinVal;
  } else {
    maxVal = liteMaxVal;
    minVal = liteMinVal;
  }
 
  // set Min-Max on % Margin
  unsigned refillFlag = 0;
  double range = maxVal-minVal;
  double margin = ( (double) MIN_MAX_MARGIN_PERCENT)/100.0;
  maxVal = maxVal + (margin* range);
  minVal = minVal - (margin* range);

  // to avoid min=max=0 and range <1 
  if (maxVal < 0.1) maxVal = 0.1;
  if ( (maxVal - minVal) < 1) minVal = (maxVal - 1);
  if (minVal<0) minVal = 0;
  range = maxVal-minVal;


  if (refillType == ForceRefill)
    refillFlag = 1;
  else {
    //change Min & Max in detector databank if +/- margin variation  
    if (fabs(syncPtr->getValMin() -minVal) > (margin* range)) refillFlag = 1;
    if (fabs(syncPtr->getValMax() -maxVal) > (margin* range)) refillFlag = 1; 
  }
  
  //Refill Histogram 
  if(refillFlag == 1) {
    double scalingFactor= (double)(X_MAX - X_MIN)/(maxVal-minVal);
    syncPtr->setValMin(minVal); 
    syncPtr->setValMax(maxVal);
    syncPtr->setScalingFactor(scalingFactor);    
    //printf("refilling - GMin = %f GMax = %f  GIdxL = %u GIdxH = %u SFctr = %f \n",liteMinVal,liteMaxVal,liteLookUpIndexLow,liteLookUpIndexHigh,scalingFactor);
    //printf("          - DMin = %f DMax = %f  DIdxL = %u DIdxH = %u            \n",darkMinVal,darkMaxVal,darkLookUpIndexLow,darkLookUpIndexHigh);

    EntryTH1F* summaryEntry = summaryLiteEntry;
    dataArrayLength = syncPtr->liteArrayLength(); 
    dataArray = syncPtr->getLiteShotArray();
    lookUpIndexHigh = liteLookUpIndexHigh;
    lookUpIndexLow  = liteLookUpIndexLow;
 
    for(unsigned j=0; j<2 ; j++) {
      if (j == 1) {
        summaryEntry = summaryDarkEntry;
        dataArrayLength = syncPtr->darkArrayLength(); 
        dataArray = syncPtr->getDarkShotArray();
        lookUpIndexHigh = darkLookUpIndexHigh;
        lookUpIndexLow  = darkLookUpIndexLow;    
      }   
     
      summaryEntry->clear();
      if( lookUpIndexHigh < lookUpIndexLow) {
        for (i=lookUpIndexLow; i < dataArrayLength; i++) 
          summaryEntry->addcontent(1.0,fabs((*(dataArray+i) - minVal) * scalingFactor) );
        for (i=0; i < lookUpIndexHigh; i++) 
          summaryEntry->addcontent(1.0,fabs((*(dataArray+i) - minVal) * scalingFactor) );
      } else {
        for (i=lookUpIndexLow; i <lookUpIndexHigh; i++) 
          summaryEntry->addcontent(1.0,fabs((*(dataArray+i) - minVal) * scalingFactor) );
      } 
    }
  } else {
    if (_darkShot)
      summaryDarkEntry->addcontent(1.0,fabs( (syncPtr->getDarkShotVal()-syncPtr->getValMin()) * syncPtr->getScalingFactor()));
    else
      summaryLiteEntry->addcontent(1.0,fabs( (syncPtr->getLiteShotVal()-syncPtr->getValMin()) * syncPtr->getScalingFactor()));
  }

 
  //auto off-by-one detection
  if (AUTO_DETECTION_ENB) {
    unsigned offByOneError = 0;
    double tolerence = margin * range;
    if ( (darkLookUpIndexHigh > 0)  || (syncPtr->statDarkShotsFull() == 1) )  {
  
      //lite & dark Curve falls in one another by small distance
      if      (fabs(liteMaxVal-darkMaxVal) < tolerence) offByOneError = 1;
      else if (fabs(liteMinVal-darkMinVal) < tolerence) offByOneError = 2;
      //lite & dark Curve intersect one another
      else if ((liteMinVal<=darkMaxVal) && (darkMaxVal<=liteMaxVal)) offByOneError = 3;
      else if ((liteMinVal<=darkMinVal) && (darkMinVal<=liteMaxVal)) offByOneError = 4;
      //lite & dark Curve closely resides at each other sides
      else if ((darkMinVal<=liteMaxVal) && ((liteMaxVal- darkMinVal)<tolerence) ) offByOneError = 5;
      else if ((darkMaxVal<=liteMinVal) && ((liteMinVal- darkMaxVal)<tolerence) ) offByOneError = 6;
      //lite & dark Curve cover each other entirely with large space
      else if ((liteMinVal<=darkMinVal) && (liteMaxVal>=darkMaxVal)) offByOneError = 7;
      else if ((darkMinVal<=liteMinVal) && (darkMaxVal>=liteMaxVal)) offByOneError = 8;
    }

      
    if (offByOneError>0) {
      switch (offByOneError) {    
        case 1: printf("\n*** OffByOneError = %u => lite & dark Curve falls in one another by small distance ***\n",offByOneError); break;
        case 2: printf("\n*** OffByOneError = %u => lite & dark Curve falls in one another by small distance ***\n",offByOneError); break;
        case 3: printf("\n*** OffByOneError = %u => lite & dark Curve intersect one another ***\n",offByOneError); break;
        case 4: printf("\n*** OffByOneError = %u => lite & dark Curve intersect one another ***\n",offByOneError); break;
        case 5: printf("\n*** OffByOneError = %u => lite & dark Curve closely resides at each other sides ***\n",offByOneError); break;
        case 6: printf("\n*** OffByOneError = %u => lite & dark Curve closely resides at each other sides ***\n",offByOneError); break;
        case 7: printf("\n*** OffByOneError = %u => lite & dark Curve cover each other entirely with large space ***\n",offByOneError); break;
        case 8: printf("\n*** OffByOneError = %u => lite & dark Curve cover each other entirely with large space ***\n",offByOneError); break;
        default: printf("\n*** OffByOneError = %u => Invalid Check ***\n",offByOneError); break;
      }
    }
  }

}

