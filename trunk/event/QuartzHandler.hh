#ifndef Ami_QuartzHandler_hh
#define Ami_QuartzHandler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class QuartzHandler : public FrameHandler {
  public:
    QuartzHandler(const Pds::DetInfo& info);
  public:
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  };
};

#endif
