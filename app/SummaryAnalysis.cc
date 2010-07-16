#include "ami/app/SummaryAnalysis.hh"
#include "ami/app/SyncAnalysis.hh"

#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"



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

static double ebeamjunk;
static bool shotCounterEnb = false;
static unsigned shotCounter = 0;

SummaryAnalysis::SummaryAnalysis():_darkShot(false),_summaryEntries(0),_analyzeCount(0),_notRefilledCount(0) { }
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

  switch(type.id())  {
    case Pds::TypeId::Id_AcqConfig:
      h = new acqDataSpace(detInfo, Pds::TypeId::Id_AcqWaveform,      Pds::TypeId::Id_AcqConfig,       payload, "Acqiris");
      //memcpy(h->configPayload(), payload, sizeof(AcqConfigType) );
      break;
    case Pds::TypeId::Id_Opal1kConfig:
      h = new opalDataSpace(detInfo, Pds::TypeId::Id_Frame,           Pds::TypeId::Id_Opal1kConfig,    payload, "Opal");
      break; 
    case Pds::TypeId::Id_FccdConfig:
      h = new SyncAnalysis(detInfo, Pds::TypeId::Id_Frame,           Pds::TypeId::Id_FccdConfig,      payload, "Fccd");
      break; 
    case Pds::TypeId::Id_TM6740Config:
      h = new pulnixDataSpace(detInfo, Pds::TypeId::Id_Frame,              Pds::TypeId::Id_TM6740Config,    payload, "Pulnix");
      break; 
    case Pds::TypeId::Id_PrincetonConfig:
      h = new princetonDataSpace(detInfo, Pds::TypeId::Id_PrincetonFrame,  Pds::TypeId::Id_PrincetonConfig, payload, "Princeton");
      //memcpy(h->configPayload(), payload, sizeof(PrincetonConfigType) );
      break;
    case Pds::TypeId::Id_pnCCDconfig:
      h = new pnccdDataSpace(detInfo, Pds::TypeId::Id_pnCCDframe,          Pds::TypeId::Id_pnCCDconfig,     payload, "PnCCD");
      //memcpy(h->configPayload(), payload, sizeof(pnCCDConfigType) );
      break;   
    case Pds::TypeId::Id_IpimbConfig:
      h = new ipimbDataSpace(detInfo, Pds::TypeId::Id_IpimbData,           Pds::TypeId::Id_IpimbConfig,     payload, "IPIMB");  
      break;
    case Pds::TypeId::Id_FEEGasDetEnergy:
      h = new gasDetectorDataSpace(detInfo, Pds::TypeId::Id_FEEGasDetEnergy, Pds::TypeId::Id_FEEGasDetEnergy, payload, "GasDetector");
      break; 
    case Pds::TypeId::Id_EBeam:
      h = new eBeamDataSpace(detInfo, Pds::TypeId::Id_EBeam,                Pds::TypeId::Id_EBeam,           payload, "EBeam");       
      break;
    case Pds::TypeId::Id_PhaseCavity:
      h = new phaseCavityDataSpace(detInfo, Pds::TypeId::Id_PhaseCavity,    Pds::TypeId::Id_PhaseCavity,     payload, "PhaseCavity"); 
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

    // Process detector's data
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
        refillPlotData(syncPtr, summaryEntry, *itSummary, DISPLAY_N_POINTS, ForceRefill) ; 
      else if(_notRefilledCount >= DISPLAY_N_POINTS)
        refillPlotData(syncPtr, summaryEntry, *itSummary, DISPLAY_N_POINTS, ForceRefill) ;      
      else {
        // initially for 100 events, check refill for every 5th event 
        if((_analyzeCount < 100) && ((_analyzeCount % 5)==0) )
          refillPlotData(syncPtr, summaryEntry, *itSummary, DISPLAY_N_POINTS, ValidateRefill) ;
        else if((_analyzeCount % REFILL_AFTER_POINTS) == 0)
          refillPlotData(syncPtr, summaryEntry, *itSummary, DISPLAY_N_POINTS, ValidateRefill) ;
        else
          summaryEntry->addcontent(1.0,fabs( (syncPtr->getLiteShotVal()-syncPtr->getValMin()) * syncPtr->getScalingFactor()));
      }

      if (AUTO_DETECTION_ENB) {
        unsigned offByOneStatus = syncPtr->getOffByOneStatus();
        if (offByOneStatus>0) {
          switch (offByOneStatus) {    
            case 1 : 
            case 2 : printf("\n ** SynEr: %s/%d =%u => Both Curves falls in one another by small distance **\n",syncPtr->detInfo.name(syncPtr->detInfo.device()),syncPtr->detInfo.devId(),offByOneStatus); break;
            case 3 : 
            case 4 : printf("\n ** SynEr: %s/%d =%u => Both Curves intersect one another **\n",syncPtr->detInfo.name(syncPtr->detInfo.device()),syncPtr->detInfo.devId(),offByOneStatus); break;
            case 5 : 
            case 6 : printf("\n ** SynEr: %s/%d =%u => Both Curves closely resides at each other sides **\n",syncPtr->detInfo.name(syncPtr->detInfo.device()),syncPtr->detInfo.devId(),offByOneStatus); break;
            case 7 :
            case 8 : printf("\n ** SynEr: %s/%d =%u => Both Curves cover each other entirely with large space  **\n",syncPtr->detInfo.name(syncPtr->detInfo.device()),syncPtr->detInfo.devId(),offByOneStatus); break;
            default: printf("\n ** SynEr: %s/%d =%u => Invalid Status Number **\n",syncPtr->detInfo.name(syncPtr->detInfo.device()),syncPtr->detInfo.devId(),offByOneStatus); break;

          }
        }
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
    //const Pds::Acqiris::ConfigV1* acqConfig = reinterpret_cast<const Pds::Acqiris::ConfigV1*>(syncPtr->configPayload()); 
    //Pds::Acqiris::DataDescV1* acqData = const_cast<Pds::Acqiris::DataDescV1*>(reinterpret_cast<const Pds::Acqiris::DataDescV1*>(syncPtr->dataPayload()));
    //const Pds::Acqiris::ConfigV1* acqConfig = &(reinterpret_cast<Ami::dataSpace<Pds::Acqiris::ConfigV1, Pds::Acqiris::DataDescV1>* >(syncPtr))->detConfig;
    //Pds::Acqiris::DataDescV1* acqData = (reinterpret_cast<Ami::dataSpace<Pds::Acqiris::ConfigV1, Pds::Acqiris::DataDescV1>* >(syncPtr))->detData;

    //typedef Ami::dataSpace<Pds::Acqiris::ConfigV1, Pds::Acqiris::DataDescV1> detSpace;
    //typedef Ami::dataSpace<AcqConfigType, Pds::Acqiris::DataDescV1> detSpace;
    //detSpace*  detDataSpace = reinterpret_cast<detSpace* >(syncPtr);
    typedef Pds::Acqiris::DataDescV1 acqDataType;
    acqDataSpace* detDataSpace = reinterpret_cast<acqDataSpace*>(syncPtr);
    const Pds::Acqiris::ConfigV1* acqConfig = &(detDataSpace->detConfig);
    //acqDataType* acqData = detDataSpace->detData;
    acqDataType* acqData = const_cast<acqDataType*>(reinterpret_cast<const acqDataType*>(syncPtr->dataPayload()));

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
    }
    syncPtr->logDataPoint((double)val, _darkShot); 
    syncPtr->setNewEventFlag(false);
  }
}


void SummaryAnalysis::processPrincetonData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    //const Pds::Princeton::ConfigV1& princetonConfig = *reinterpret_cast<const Pds::Princeton::ConfigV1*>(syncPtr->configPayload());
    princetonDataSpace* detDataSpace = reinterpret_cast<princetonDataSpace*>(syncPtr);
    const PrincetonConfigType& princetonConfig = detDataSpace->detConfig;
 
    const Pds::Princeton::FrameV1&  princetonFrame   = *reinterpret_cast<const Pds::Princeton::FrameV1*>(syncPtr->dataPayload());
    const uint16_t* dataArray = reinterpret_cast<const uint16_t*>(princetonFrame.data());
    unsigned totalPixels = princetonConfig.width()*princetonConfig.height();

    unsigned val = 0;   
    for (unsigned i = 0 ; i<totalPixels ; i++) 
      val = val + (*(dataArray+i) ); 
    
    syncPtr->logDataPoint((double)val, _darkShot);
    syncPtr->setNewEventFlag(false); 
    //printf(" 2 FrData: Width = %u Height= %u pixel_50.30 = %u val = %u\n", princetonConfig.width(), princetonConfig.height(), *(dataArray+ ( princetonConfig.width() *30 + 50) ), val ); 

  }
 
}

void SummaryAnalysis::processPnccdData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    //const Pds::PNCCD::ConfigV1& pnccdConfig  = *reinterpret_cast<const Pds::PNCCD::ConfigV1*>(syncPtr->configPayload());
    pnccdDataSpace* detDataSpace = reinterpret_cast<pnccdDataSpace*>(syncPtr);
    const pnCCDConfigType& pnccdConfig = detDataSpace->detConfig;
 
    const Pds::PNCCD::FrameV1*  pnccdFrame   = reinterpret_cast<const Pds::PNCCD::FrameV1*>(syncPtr->dataPayload());
    const uint16_t* dataArray = reinterpret_cast<const uint16_t*>(pnccdFrame->data());

    unsigned val = 0;
    for (unsigned j=0; j<4 ; j++ ) {                // for all 4 quadrants in sequence UpL+LoL+LoR+UpR
      unsigned dataSize = pnccdFrame->sizeofData(pnccdConfig);
      for(unsigned i= 0; i< dataSize ; i++) {
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
    ipimbDataSpace* detDataSpace = reinterpret_cast<ipimbDataSpace*>(syncPtr);
    const Pds::Ipimb::ConfigV1* ipimbConfig = &(detDataSpace->detConfig); //*reinterpret_cast<const Pds::Ipimb::ConfigV1*>(payload);
    //const Pds::Ipimb::ConfigV1& ipimbConfig   = *reinterpret_cast<const Pds::Ipimb::ConfigV1*>(syncPtr->configPayload());

    const Pds::Ipimb::DataV1& ipimbData = *reinterpret_cast<const Pds::Ipimb::DataV1*>(syncPtr->dataPayload());
    //ipimb all 4 channels voltage abs() addition  -- Do we need average ??
    double val =  fabs(ipimbData.channel0Volts()) + fabs(ipimbData.channel1Volts()) + fabs(ipimbData.channel2Volts()) + fabs(ipimbData.channel3Volts()) ; 
    syncPtr->logDataPoint(val, _darkShot);
    syncPtr->setNewEventFlag(false); 


  //off by one detection
  if (_darkShot)  shotCounterEnb = true;
  if (shotCounterEnb)  shotCounter++;
  
  if (shotCounter == 10) {
    double*   dataArray1 = syncPtr->getLiteShotArray();
    unsigned  index1     = syncPtr->getLiteShotIndex() - 10; 
    double*   dataArray2 = syncPtr->getDarkShotArray();
    unsigned  index2     = syncPtr->getDarkShotIndex(); 
    if (index1 < 10) printf("$$$$$$$$ index low --  may segfault\n"); 
    else {
      printf("\n####Good Prev = \n ");
      for (int i =-9; i <10;i++)
        printf(" %dth=%f \t",i,*(dataArray1+index1+i));
      printf("\n####Dark Prev = \n ");
      for (unsigned i=0; i<=index2;i++)
        printf(" %dth=%f \t",i,*(dataArray2+i)); 
      printf("#### bias = %f \n ",ipimbConfig->diodeBias() );  
    } 
    shotCounterEnb = false;
    shotCounter = 0;
  }


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
    ebeamjunk=val;
  } 
}

void SummaryAnalysis::processPhaseCavityData(SyncAnalysis* syncPtr)
{
  if (syncPtr->newEvent()) {
    const Pds::BldDataPhaseCavity& phaseCavityData = *reinterpret_cast<const Pds::BldDataPhaseCavity*>(syncPtr->dataPayload());
    double val = fabs(phaseCavityData.fCharge1) + fabs(phaseCavityData.fCharge2); 
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




void SummaryAnalysis::refillPlotData(SyncAnalysis* syncPtr, EntryTH1F* summaryLiteEntry, EntryTH1F* summaryDarkEntry, unsigned points, unsigned refillType)
{

  findMinMaxRange(syncPtr, points);


  if (refillType == ForceRefill)
    fillPlots(syncPtr, summaryLiteEntry, summaryDarkEntry);
  //check change Min & Max in detector databank if +/- margin variation 
  else if ( (fabs(syncPtr->getValMin() - _minVal) > (_margin* _range)) ||
            (fabs(syncPtr->getValMax() - _maxVal) > (_margin* _range))  )
    fillPlots(syncPtr, summaryLiteEntry, summaryDarkEntry);
  else {
    if (_darkShot)
      summaryDarkEntry->addcontent(1.0,fabs( (syncPtr->getDarkShotVal()-syncPtr->getValMin()) * syncPtr->getScalingFactor()));
    else
      summaryLiteEntry->addcontent(1.0,fabs( (syncPtr->getLiteShotVal()-syncPtr->getValMin()) * syncPtr->getScalingFactor()));
  }

  if (AUTO_DETECTION_ENB)
    autoOffByOneDetection(syncPtr);

 } 



void SummaryAnalysis::findMinMaxRange(SyncAnalysis* syncPtr, unsigned points)
{
  unsigned dataArrayLength = syncPtr->liteArrayLength(); 
  double*  dataArray   = syncPtr->getLiteShotArray();
  unsigned statShotsFull = syncPtr->statLiteShotsFull();
  unsigned lookUpIndexHigh = syncPtr->getLiteShotIndex(); 
  unsigned lookUpIndexLow  = 0;
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
    _minVal = *(dataArray + lookUpIndexLow);
    _maxVal = *(dataArray + lookUpIndexLow);
  
    if( lookUpIndexHigh < lookUpIndexLow) {
      for (i=lookUpIndexLow; i < dataArrayLength; i++) {
        if (_minVal > *(dataArray+i)) _minVal = *(dataArray+i);
        if (_maxVal < *(dataArray+i)) _maxVal = *(dataArray+i);
      }
      for (i=0; i < lookUpIndexHigh; i++) {
        if (_minVal > *(dataArray+i)) _minVal = *(dataArray+i);
        if (_maxVal < *(dataArray+i)) _maxVal = *(dataArray+i);
      }
    }
    else {
      for (i=lookUpIndexLow; i <lookUpIndexHigh; i++) {
        if (_minVal > *(dataArray+i)) _minVal = *(dataArray+i);
        if (_maxVal < *(dataArray+i)) _maxVal = *(dataArray+i);
      }
    }


    if (j == 1) {
      _darkMinVal          = _minVal;
      _darkMaxVal          = _maxVal; 
      _darkLookUpIndexHigh = lookUpIndexHigh;    
      _darkLookUpIndexLow  = lookUpIndexLow;
    } else {
      _liteMinVal          = _minVal;
      _liteMaxVal          = _maxVal; 
      _liteLookUpIndexHigh = lookUpIndexHigh;    
      _liteLookUpIndexLow  = lookUpIndexLow;
    } 
  }


  // consider darkMin & darkMax only if dark shot is present
  if ( (_darkLookUpIndexHigh > 0)  || (syncPtr->statDarkShotsFull() == 1) )  {
    _maxVal = (_liteMaxVal >= _darkMaxVal) ? _liteMaxVal : _darkMaxVal;
    _minVal = (_liteMinVal <= _darkMinVal) ? _liteMinVal : _darkMinVal;
  } else {
    _maxVal = _liteMaxVal;
    _minVal = _liteMinVal;
  }
 
  // set Min-Max on % Margin
  _range = _maxVal- _minVal;
  _margin = ( (double) MIN_MAX_MARGIN_PERCENT)/100.0;
  _maxVal = _maxVal + (_margin* _range);
  _minVal = _minVal - (_margin* _range);

  // to avoid min=max=0 and range <1 
  if (_maxVal < 0) _maxVal = 0;
  //if ( (_maxVal - _minVal) < 1) _minVal = (_maxVal - 1);
  if (_minVal<0) _minVal = 0;
  _range = _maxVal- _minVal;

}

void SummaryAnalysis::fillPlots(SyncAnalysis* syncPtr, EntryTH1F* summaryLiteEntry, EntryTH1F* summaryDarkEntry)
{

    double scalingFactor = (double)(X_MAX - X_MIN)/(_maxVal-_minVal);
    syncPtr->setValMin(_minVal); 
    syncPtr->setValMax(_maxVal);
    syncPtr->setScalingFactor(scalingFactor);    
    //printf("refilling - GMin = %f GMax = %f  GIdxL = %u GIdxH = %u SFctr = %f \n",_liteMinVal,_liteMaxVal,_liteLookUpIndexLow,_liteLookUpIndexHigh,scalingFactor);
    //printf("          - DMin = %f DMax = %f  DIdxL = %u DIdxH = %u            \n",_darkMinVal,_darkMaxVal,_darkLookUpIndexLow,_darkLookUpIndexHigh);

    EntryTH1F* summaryEntry = summaryLiteEntry;
    unsigned dataArrayLength = syncPtr->liteArrayLength(); 
    double*  dataArray = syncPtr->getLiteShotArray();
    unsigned lookUpIndexHigh = _liteLookUpIndexHigh;
    unsigned lookUpIndexLow  = _liteLookUpIndexLow;
    unsigned i=0;
 
    for(unsigned j=0; j<2 ; j++) {
      if (j == 1) {
        summaryEntry = summaryDarkEntry;
        dataArrayLength = syncPtr->darkArrayLength(); 
        dataArray = syncPtr->getDarkShotArray();
        lookUpIndexHigh = _darkLookUpIndexHigh;
        lookUpIndexLow  = _darkLookUpIndexLow;    
      }   
     
      summaryEntry->clear();
      if( lookUpIndexHigh < lookUpIndexLow) {
        for (i=lookUpIndexLow; i < dataArrayLength; i++) 
          summaryEntry->addcontent(1.0,fabs((*(dataArray+i) - _minVal) * scalingFactor) );
        for (i=0; i < lookUpIndexHigh; i++) 
          summaryEntry->addcontent(1.0,fabs((*(dataArray+i) - _minVal) * scalingFactor) );
      } else {
        for (i=lookUpIndexLow; i <lookUpIndexHigh; i++) 
          summaryEntry->addcontent(1.0,fabs((*(dataArray+i) - _minVal) * scalingFactor) );
      } 
    }

}


void SummaryAnalysis::autoOffByOneDetection(SyncAnalysis* syncPtr)
{
    unsigned offByOneStatus = 0;
    double tolerence = _margin * _range;
    if ( (_darkLookUpIndexHigh > 0)  || (syncPtr->statDarkShotsFull() == 1) )  {
  
      //lite & dark Curve falls in one another by small distance
      if      (fabs(_liteMaxVal- _darkMaxVal) < tolerence) offByOneStatus = 1;
      else if (fabs(_liteMinVal- _darkMinVal) < tolerence) offByOneStatus = 2;
      //lite & dark Curve intersect one another
      else if ((_liteMinVal<= _darkMaxVal) && (_darkMaxVal<= _liteMaxVal)) offByOneStatus = 3;
      else if ((_liteMinVal<= _darkMinVal) && (_darkMinVal<= _liteMaxVal)) offByOneStatus = 4;
      //lite & dark Curve closely resides at each other sides
      else if ((_darkMinVal<= _liteMaxVal) && ((_liteMaxVal- _darkMinVal)<tolerence) ) offByOneStatus = 5;
      else if ((_darkMaxVal<= _liteMinVal) && ((_liteMinVal- _darkMaxVal)<tolerence) ) offByOneStatus = 6;
      //lite & dark Curve cover each other entirely with large space
      else if ((_liteMinVal<= _darkMinVal) && (_liteMaxVal>= _darkMaxVal)) offByOneStatus = 7;
      else if ((_darkMinVal<= _liteMinVal) && (_darkMaxVal>= _liteMaxVal)) offByOneStatus = 8;
    }

    syncPtr->setOffByOneStatus(offByOneStatus);

}

