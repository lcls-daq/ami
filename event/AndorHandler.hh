#ifndef Ami_AndorHandler_hh
#define Ami_AndorHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pds/config/AndorConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class EntryImage;

  class AndorHandler : public EventHandler {
  public:
    AndorHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~AndorHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  public:
    virtual void _configure(Pds::TypeId type, const void* payload, const Pds::ClockTime& t);    
    virtual void _calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t);
    virtual void _event    (Pds::TypeId type, const void* payload, const Pds::ClockTime& t);
    virtual void _damaged  ();
  private:
    //AndorHandler(const Pds::DetInfo& info, const EntryImage*);
    AndorConfigType  _config;
    FeatureCache&       _cache;
    int                 _iCacheIndexTemperature;
    EntryImage*         _entry;
  };
};

#endif
