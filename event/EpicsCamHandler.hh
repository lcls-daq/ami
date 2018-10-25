#ifndef Ami_EpicsCamHandler_hh
#define Ami_EpicsCamHandler_hh

#include "ami/event/FrameHandler.hh"
#include "pdsdata/psddl/lusi.ddl.h"

namespace Ami {
  class EpicsCamHandler : public FrameHandler {
  public:
    EpicsCamHandler(const Pds::DetInfo& info);
  public:
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  private:
    Pds::Lusi::PimImageConfigV1 _scale;
  };
};

#endif
