#ifndef Ami_TM6740Handler_hh
#define Ami_TM6740Handler_hh

#include "ami/event/FrameHandler.hh"
#include "pdsdata/psddl/pulnix.ddl.h"
#include "pdsdata/psddl/lusi.ddl.h"

namespace Ami {
  class TM6740Handler : public FrameHandler {
  public:
    TM6740Handler(const Pds::DetInfo& info);
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  private:
    Pds::Lusi::PimImageConfigV1 _scale;
  };
};

#endif
