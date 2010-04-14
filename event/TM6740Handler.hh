#ifndef Ami_TM6740Handler_hh
#define Ami_TM6740Handler_hh

#include "ami/event/FrameHandler.hh"
#include "pdsdata/pulnix/TM6740ConfigV1.hh"

namespace Ami {
  class TM6740Handler : public FrameHandler {
  public:
    TM6740Handler(const Pds::DetInfo& info) :
      FrameHandler(info,
		   Pds::Pulnix::TM6740ConfigV1::Column_Pixels,
		   Pds::Pulnix::TM6740ConfigV1::Row_Pixels) {}
  };
};

#endif
