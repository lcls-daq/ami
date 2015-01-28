#ifndef Ami_PimaxHandler_hh
#define Ami_PimaxHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/pimax.ddl.h"

namespace Ami {
  class EntryImage;

  class PimaxHandler : public EventHandler {
  public:
    PimaxHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~PimaxHandler();
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
    //PimaxHandler(const Pds::DetInfo& info, const EntryImage*);
    Pds::Pimax::ConfigV1  _config;
    FeatureCache&       _cache;
    int                 _iCacheIndexTemperature;
    EntryImage*         _entry;
    EntryImage*         _pentry;
    ndarray<unsigned,2> _offset;
    unsigned            _options;
  };
};

#endif
