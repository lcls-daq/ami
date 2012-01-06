#ifndef Ami_PhasicsHandler_hh
#define Ami_PhasicsHandler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class PhasicsHandler : public FrameHandler {
  public:
    PhasicsHandler(const Pds::DetInfo& info);
  protected:
    void _configure(const void*, const Pds::ClockTime&);
  };
};

#endif
