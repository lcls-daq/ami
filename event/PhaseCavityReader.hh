#ifndef Ami_PhaseCavityReader_hh
#define Ami_PhaseCavityReader_hh

#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

namespace Ami {

  class PhaseCavityReader : public EventHandlerF {
  public:
    PhaseCavityReader(FeatureCache&);
    ~PhaseCavityReader();
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         rename  (const char*);
    void         reset   ();
  };

};

#endif
