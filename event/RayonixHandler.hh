#ifndef Ami_RayonixHandler_hh
#define Ami_RayonixHandler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class RayonixHandler : public FrameHandler {

    enum { n_pixels_fast = 7680 };
    enum { n_pixels_slow = 7680 };

  public:
    RayonixHandler(const Pds::DetInfo& info);
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  };
};

#endif
