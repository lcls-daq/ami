#ifndef Ami_ArchonHandler_hh
#define Ami_ArchonHandler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class ArchonHandler : public FrameHandler {
  public:
    ArchonHandler(const Pds::DetInfo& info);
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  };
};

#endif
