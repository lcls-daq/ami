#ifndef Ami_FccdHandler_hh
#define Ami_FccdHandler_hh

#include "ami/event/FrameHandler.hh"
#include "pdsdata/fccd/FccdConfigV1.hh"

namespace Ami {
  class FccdHandler : public FrameHandler {
  public:
    FccdHandler(const Pds::DetInfo& info) :
      FrameHandler(info,
		   Pds::FCCD::FccdConfigV1::Column_Pixels,
		   Pds::FCCD::FccdConfigV1::Row_Pixels) {}
  };
};

#endif
