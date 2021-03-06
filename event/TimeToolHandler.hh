#ifndef Ami_TimeToolHandler_hh
#define Ami_TimeToolHandler_hh

#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

namespace Pds {
  class Dgram;
};

namespace Ami {

  class TimeToolHandler : public EventHandlerF {
  public:
    TimeToolHandler(const Pds::Src&, FeatureCache&);
    ~TimeToolHandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    bool         used    () const { return true; }
    void         rename  (const char*);
  };

};

#endif
