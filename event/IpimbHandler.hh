#ifndef Ami_IpimbReader_hh
#define Ami_IpimbReader_hh

#include "ami/event/EventHandlerF.hh"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class IpimbHandler : public EventHandlerF {
  public:
    IpimbHandler(const Pds::DetInfo&, FeatureCache&);
    ~IpimbHandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    bool         used    () const { return true; }
    void         rename  (const char*);
  private:
    enum { NChannels=4 };
  };

};

#endif
