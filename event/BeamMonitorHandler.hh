#ifndef Ami_BeamMonitorHandler_hh
#define Ami_BeamMonitorHandler_hh

#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

namespace Pds { class BldInfo; };

namespace Ami {

  class BeamMonitorHandler : public EventHandlerF {
  public:
    BeamMonitorHandler(const Pds::BldInfo&, FeatureCache&);
    ~BeamMonitorHandler();
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
