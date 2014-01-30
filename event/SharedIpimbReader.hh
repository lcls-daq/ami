#ifndef Ami_SharedIpimbReader_hh
#define Ami_SharedIpimbReader_hh

#include "ami/event/EventHandlerF.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/BldInfo.hh"

namespace Ami {

  class SharedIpimbReader : public EventHandlerF {
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
    void         rename  (const char*);
    void         reset   ();
  private:
    enum { NChannels= 15 };
    int                     _index[NChannels];
  };

};

#endif
