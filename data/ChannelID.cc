#include "ChannelID.hh"

#include <string.h>
#include <stdio.h>

using namespace Pds;

static char _buffer[128];

#define NoChannel(title) { strcpy(_buffer,title); }
#define AppendChannel(title) { sprintf(_buffer,"%s_%d",title,channel+1); }
#define AcqChannel(title) {			\
    if (info.device()==DetInfo::Acqiris)	\
      AppendChannel(title);			\
  }
#define OpalChannel(title) {			\
    if (info.device()==DetInfo::Opal1000)	\
      AppendChannel(title);			\
  }
#define AcqDetector(title) {			\
    if (info.device()==DetInfo::Acqiris)	\
      NoChannel(title);				\
  }
#define OpalDetector(title) {				\
    if (info.device()==DetInfo::Opal1000) {		\
      if (info.devId()==0)				\
	strcpy(_buffer,title);				\
      else						\
	sprintf(_buffer,"%s_%d",title,info.devId()+1);	\
    }							\
  }

const char* Ami::ChannelID::name(const Pds::DetInfo& info,
				 unsigned channel)
{
  *_buffer = 0;
  switch(info.detector()) {
    //  AMO Detectors
  case DetInfo::AmoITof  : AcqDetector("ITOF"); break;
  case DetInfo::AmoIms   : AcqChannel("IMS"); break;
  case DetInfo::AmoETof  : AcqChannel("ETOF"); break;
  case DetInfo::AmoGasdet: AcqChannel("GASDET"); break;
  case DetInfo::AmoMbes  : AcqChannel("MBES"); break;
  case DetInfo::AmoBps   : OpalChannel("BPS"); break;
    //  CAMP Detectors
  case DetInfo::Camp     : AcqChannel("ACQ"); OpalDetector("VMI"); break;
    //  Others
  default: break;
  }
  return _buffer;
}   

