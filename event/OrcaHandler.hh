#ifndef Ami_OrcaHandler_hh
#define Ami_OrcaHandler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class OrcaHandler : public FrameHandler {
  public:
    OrcaHandler(const Pds::DetInfo& info);
  protected:
    void _configure(Pds::TypeId, 
		    const void* payload, const Pds::ClockTime& t);
  };
};

#endif
