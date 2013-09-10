#ifndef Ami_SharedIpimbReader_hh
#define Ami_SharedIpimbReader_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/BldInfo.hh"

namespace Ami {

  class SharedIpimbReader : public EventHandler {
  public:
    SharedIpimbReader(const Pds::BldInfo&, FeatureCache&);
    ~SharedIpimbReader();
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         reset   ();
    void         rename  (const char*);
  private:
    FeatureCache& _cache;
    enum { NChannels= 15 };
    int                     _index[NChannels];
  };

};

#endif
