#ifndef Ami_FEEGasDetEnergyReader_hh
#define Ami_FEEGasDetEnergyReader_hh

#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

namespace Ami {

  class FEEGasDetEnergyReader : public EventHandlerF {
  public:
    FEEGasDetEnergyReader(FeatureCache&);
    ~FEEGasDetEnergyReader();
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         reset   ();
    void         rename  (const char*);
  private:
    int           _index;
    int           _version;
  };

};

#endif
