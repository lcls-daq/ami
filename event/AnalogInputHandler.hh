#ifndef Ami_AnalogInputHandler_hh
#define Ami_AnalogInputHandler_hh

#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

namespace Pds { class BldInfo; };

namespace Ami {

  class AnalogInputHandler : public EventHandlerF {
  public:
    AnalogInputHandler(const Pds::BldInfo&, FeatureCache&);
    ~AnalogInputHandler();
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         rename  (const char*);
  };

};

#endif
