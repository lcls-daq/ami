#ifndef Ami_FrameFexHandler_hh
#define Ami_FrameFexHandler_hh

#include "ami/event/EventHandlerF.hh"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class FrameFexHandler : public EventHandlerF {
  public:
    FrameFexHandler(const DetInfo&, FeatureCache&);
    ~FrameFexHandler();
  public:
    void   _calibrate(const void* payload, const Pds::ClockTime& t);
    void   _configure(const void* payload, const Pds::ClockTime& t);
    void   _event    (const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         rename  (const char*);
  };

};

#endif
