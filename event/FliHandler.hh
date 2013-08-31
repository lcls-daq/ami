#ifndef Ami_FliHandler_hh
#define Ami_FliHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pds/config/FliConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class EntryImage;

  class FliHandler : public EventHandler {
  public:
    FliHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~FliHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  public:
    virtual void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);    
    virtual void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    virtual void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    virtual void _damaged  ();
  private:
    //FliHandler(const Pds::DetInfo& info, const EntryImage*);
    FliConfigType  _config;
    FeatureCache&       _cache;
    int                 _iCacheIndexTemperature;
    EntryImage*         _entry;
  };
};

#endif
