#ifndef Ami_SharedUsdUsbHandler_hh
#define Ami_SharedUsdUsbHandler_hh

#include "ami/event/EventHandlerF.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/BldInfo.hh"

namespace Ami {

  class SharedUsdUsbHandler : public EventHandlerF {
  public:
    SharedUsdUsbHandler(const Pds::BldInfo&, FeatureCache&);
    ~SharedUsdUsbHandler();
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         rename  (const char*);
  private:
    const static unsigned NCHAN = 4;
    const static unsigned DCHAN = 8;
    const static unsigned NAME_LEN = 64;
    char      _aliases[NCHAN][NAME_LEN];
    unsigned  _alias_mask;
    bool      _use_alias(const unsigned index) const;
  };

};

#endif
