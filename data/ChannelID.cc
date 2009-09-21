#include "ChannelID.hh"

#include <stdio.h>

static char _buffer[128];

const char* Ami::ChannelID::name(const Pds::DetInfo& info,
				 unsigned channel)
{
  if (info.detector()==Pds::DetInfo::AmoETof)
    sprintf(_buffer,"%s Channel %d",Pds::DetInfo::name(info),channel);
  else
    sprintf(_buffer,"%s",Pds::DetInfo::name(info));
  return _buffer;
}
