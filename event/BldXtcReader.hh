#ifndef Ami_BldXtcReader_hh
#define Ami_BldXtcReader_hh

#include "ami/event/EventHandler.hh"

#include "ami/data/FeatureCache.hh"

namespace Ami {

  class BldXtcReader : public EventHandler {
  public:
    BldXtcReader(FeatureCache&);
    ~BldXtcReader();
  public:
    void   _configure(const void* payload, const Pds::ClockTime& t);
    void   _calibrate(const void* payload, const Pds::ClockTime& t);
    void   _event    (const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         reset   ();
  private:
    FeatureCache& _cache;
    int           _index;
  };

};

#endif
