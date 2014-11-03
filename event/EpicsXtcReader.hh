#ifndef Ami_EpicsXtcReader_hh
#define Ami_EpicsXtcReader_hh

#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

namespace Pds {
  class Dgram;
};

namespace Ami {

  class EpicsXtcReader : public EventHandlerF {
  public:
    EpicsXtcReader(const Pds::Src&, FeatureCache&);
    ~EpicsXtcReader();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         rename  (const char*);
  private:
    enum { MaxPvs=1024 };
    int           _indexpv[MaxPvs];
  };

};

#endif
