#ifndef Ami_QuartzHandler_hh
#define Ami_QuartzHandler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class QuartzHandler : public FrameHandler {
  public:
    QuartzHandler(const Pds::DetInfo& info);
  };
};

#endif
