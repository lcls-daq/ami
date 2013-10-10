#ifndef Ami_L3THandler_hh
#define Ami_L3THandler_hh

#include "ami/event/EventHandler.hh"

namespace Pds {
  class Dgram;
};

namespace Ami {
  class FeatureCache;
  class L3THandler : public EventHandler {
  public:
    L3THandler(FeatureCache&);
    ~L3THandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  protected:
    void   _calibrate(const void* payload, const Pds::ClockTime& t) {}
    void   _configure(const void* payload, const Pds::ClockTime& t) {}
    void   _event    (const void* payload, const Pds::ClockTime& t) {}
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         reset   ();
    void         rename  (const char*);
  private:
    FeatureCache& _cache;
    int _index;
  };

};

#endif
