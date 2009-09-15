#include "ChannelID.hh"

#include <stdio.h>

static char _buffer[128];

const char* Ami::ChannelID::name(const Pds::DetInfo& info,
				 unsigned channel)
{
  sprintf(_buffer,"%s Channel %d",Pds::DetInfo::name(info),channel);
  return _buffer;
}
