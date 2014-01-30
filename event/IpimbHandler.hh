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
    IpimbHandler(const DetInfo&, FeatureCache&);
    ~IpimbHandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    bool         used    () const { return true; }
    void         rename  (const char*);
    void         reset   ();
  private:
    enum { NChannels=4 };
    int                  _index[NChannels*2];
  };

};

#endif
