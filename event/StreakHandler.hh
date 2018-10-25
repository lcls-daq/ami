#ifndef Ami_StreakHandler_hh
#define Ami_StreakHandler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class StreakHandler : public FrameHandler {
  public:
    StreakHandler(const Pds::DetInfo& info);
  public:
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  private:
    void set_units(const char* units);
  private:
    enum { UnitsSize=64 };
    double _scale;
    char   _units[UnitsSize];
  };
};

#endif
