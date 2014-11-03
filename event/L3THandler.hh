#ifndef Ami_L3THandler_hh
#define Ami_L3THandler_hh

#include "ami/event/EventHandlerF.hh"

namespace Pds {
  class Dgram;
};

namespace Ami {
  class FeatureCache;
  class L3THandler : public EventHandlerF {
  public:
    L3THandler(FeatureCache&);
    ~L3THandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  protected:
    void   _calibrate(const void* payload, const Pds::ClockTime& t) {}
    void   _configure(const void* payload, const Pds::ClockTime& t) {}
    void   _event    (const void* payload, const Pds::ClockTime& t) {}
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         rename  (const char*);
  };

};

#endif
