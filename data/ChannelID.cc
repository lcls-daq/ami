#include "ChannelID.hh"

#include <string.h>
#include <stdio.h>

using namespace Pds;

static char _buffer[128];

#define NoChannel(title) { strcpy(_buffer,title); }
#define AppendChannel(title) { sprintf(_buffer,"%s_%d",title,channel+1); }
#define AcqChannel(title)			\
    if (info.device()==DetInfo::Acqiris)	\
      AppendChannel(title)			
#define OpalChannel(title)  			\
    if (info.device()==DetInfo::Opal1000)	\
      AppendChannel(title)			
#define AcqDetector(title) 			\
    if (info.device()==DetInfo::Acqiris)	\
      NoChannel(title);				
#define OpalDetector(title) 				\
    if (info.device()==DetInfo::Opal1000) {		\
      if (info.devId()==0)				\
	strcpy(_buffer,title);				\
      else						\
	sprintf(_buffer,"%s_%d",title,info.devId()+1);	\
    }							
#define PnccdDetector 				     \
    if (info.device()==DetInfo::pnCCD)		     \
      sprintf(_buffer,"pnCCD_%d",info.devId()+1);    

static void _default(char* b, const DetInfo& info, unsigned channel)
{
  if (info.device()==DetInfo::Acqiris)
    sprintf(b,"UNK_%s_%s_%d",
	    DetInfo::name(info.detector()),
	    DetInfo::name(info.device  ()), 
	    channel+1);
  else
    sprintf(b,"UNK_%s_%s",
	    DetInfo::name(info.detector()),
	    DetInfo::name(info.device  ()));
}

const char* Ami::ChannelID::name(const Pds::DetInfo& info,
				 unsigned channel)
{
  *_buffer = 0;
  switch(info.detector()) {
    //  AMO Detectors
  case DetInfo::AmoITof  : AcqChannel("ITOF"); break;
  case DetInfo::AmoIms   : AcqChannel("IMS"); break;
  case DetInfo::AmoETof  : AcqChannel("ETOF"); break;
  case DetInfo::AmoGasdet: AcqChannel("GASDET"); break;
  case DetInfo::AmoMbes  : AcqChannel("MBES"); break;
  case DetInfo::AmoBps   : OpalChannel("BPS"); break;
    //  CAMP Detectors
  case DetInfo::Camp     : 
    AcqChannel("ACQ")
    else OpalDetector("VMI")
    else PnccdDetector
    else _default(_buffer,info,channel);
    break;
    //  Others
  case DetInfo::SxrBeamline:
    switch(info.device()) {
    case DetInfo::Opal1000:
      switch(info.devId()) {
      case 0 : strcpy(_buffer,"TSS_Opal"); break;
      case 1 : strcpy(_buffer,"EXS_Opal"); break;
      default: _default(_buffer,info,channel); break;
      }
      break;
    default: _default(_buffer,info,channel); break;
    }
    break;
  case DetInfo::SxrEndstation:
    switch(info.device()) {
    case DetInfo::Acqiris : AppendChannel("ACQ"); break;
    case DetInfo::Opal1000: AppendChannel("OPAL"); break;
    case DetInfo::Princeton:
      sprintf(_buffer,"PI_%d",info.devId()+1);
      break;
    default: _default(_buffer,info,channel); break;
    }
    break;    
  default:
    switch(info.device()) {
    case DetInfo::TM6740: sprintf(_buffer,"%sCvd",Pds::DetInfo::name(info.detector())); break;
    default: _default(_buffer,info,channel); break;
    } 
    break;
  }
  return _buffer;
}   

