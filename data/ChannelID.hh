#ifndef Ami_ChannelID_hh
#define Ami_ChannelID_hh

#include "ami/data/Channel.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class ChannelID {
  public:
    static const char* name(const Pds::DetInfo& info);
    static const char* name(const Pds::DetInfo& info, Channel);
  };
};

#endif
