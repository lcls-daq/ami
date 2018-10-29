#ifndef Ami_RayonixHandler_hh
#define Ami_RayonixHandler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class RayonixHandler : public FrameHandler {

  public:
    RayonixHandler(const Pds::DetInfo& info);
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  };
};

#endif
