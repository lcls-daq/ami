#ifndef Ami_Fccd960Handler_hh
#define Ami_Fccd960Handler_hh

#include "ami/event/FrameHandler.hh"

namespace Ami {
  class Fccd960Handler : public FrameHandler {
  public:
    Fccd960Handler(const Pds::DetInfo& info);
  };
};

#endif
