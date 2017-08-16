#ifndef Ami_ZylaHandler_hh
#define Ami_ZylaHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/zyla.ddl.h"

namespace Ami {
  class EntryImage;

  class ZylaHandler : public EventHandler {
  public:
    ZylaHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~ZylaHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  public:
    void         rename(const char*);
  public:
    virtual void _configure(Pds::TypeId type, const void* payload, const Pds::ClockTime& t);    
    virtual void _calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t);
    virtual void _event    (Pds::TypeId type, const void* payload, const Pds::ClockTime& t);
    virtual void _damaged  ();
  private:
    void _load_pedestals();
  private:
    //ZylaHandler(const Pds::DetInfo& info, const EntryImage*);
    Pds::Xtc*           _configtc;
    FeatureCache&       _cache;
    EntryImage*         _entry;
    EntryImage*         _pentry;
    ndarray<unsigned,2> _offset;
    unsigned            _options;
  };
};

#endif
