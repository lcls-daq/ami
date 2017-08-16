#ifndef Ami_IpmFexHandler_hh
#define Ami_IpmFexHandler_hh

#include "ami/event/EventHandlerF.hh"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class IpmFexHandler : public EventHandlerF {
  public:
    IpmFexHandler(const Pds::DetInfo&, FeatureCache&);
    ~IpmFexHandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         rename  (const char*);
  };

};

#endif
