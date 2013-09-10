#ifndef Ami_PrincetonHandler_hh
#define Ami_PrincetonHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class EntryImage;

  class PrincetonHandler : public EventHandler {
  public:
    PrincetonHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~PrincetonHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
    void         rename(const char*);
  public:
    virtual void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    virtual void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    virtual void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    virtual void _damaged  ();
  private:
    //PrincetonHandler(const Pds::DetInfo& info, const EntryImage*);
    Xtc*                 _configtc;
    FeatureCache&        _cache;
    int                  _iCacheIndexTemperature;
    EntryImage*          _entry;
  };
};

#endif
