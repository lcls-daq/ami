#ifndef Ami_GMDReader_hh
#define Ami_GMDReader_hh

#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

namespace Ami {

  class GMDReader : public EventHandlerF {
  public:
    GMDReader(FeatureCache&);
    ~GMDReader();
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         rename  (const char*);
  private:
    FeatureCache& _cache;
    int           _index;
  };

};

#endif
