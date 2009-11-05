#include "ChannelID.hh"

#include <stdio.h>

static char _buffer[128];

const char* Ami::ChannelID::name(const Pds::DetInfo& info,
				 unsigned channel)
{
  if (info.detector()==Pds::DetInfo::AmoETof ||
      info.detector()==Pds::DetInfo::AmoGasdet ||
      info.detector()==Pds::DetInfo::AmoMbes)
    sprintf(_buffer,"%s Channel %d",Pds::DetInfo::name(info),channel+1);
  else if (info.detector()==Pds::DetInfo::AmoITof &&
	   channel==1)
    sprintf(_buffer,"SyncdTrigger");
  else
    sprintf(_buffer,"%s",Pds::DetInfo::name(info));
  return _buffer;
}
