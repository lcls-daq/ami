#ifndef Ami_Opal1kHandler_hh
#define Ami_Opal1kHandler_hh

#include "ami/event/FrameHandler.hh"
#include "pdsdata/opal1k/ConfigV1.hh"

namespace Ami {
  class Opal1kHandler : public FrameHandler {
  public:
    Opal1kHandler(const Pds::DetInfo& info) : 
      FrameHandler(info, 
		   Pds::Opal1k::ConfigV1::Column_Pixels,
		   Pds::Opal1k::ConfigV1::Row_Pixels) {}
  };
};

#endif
