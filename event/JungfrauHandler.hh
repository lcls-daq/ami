#ifndef Ami_JungfrauHandler_hh
#define Ami_JungfrauHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/jungfrau.ddl.h"

namespace Ami {
  class EntryImage;

  class JungfrauHandler : public EventHandler {
  public:
    JungfrauHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~JungfrauHandler();
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
    Pds::Xtc*           _configtc;
    FeatureCache&       _cache;
    EntryImage*         _entry;
    EntryImage*         _pentry;
    ndarray<unsigned,2> _offset;
    unsigned            _options;
  };
};

#endif
