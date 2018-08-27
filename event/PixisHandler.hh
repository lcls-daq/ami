#ifndef Ami_PixisHandler_hh
#define Ami_PixisHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/pixis.ddl.h"

namespace Ami {
  class EntryImage;

  class PixisHandler : public EventHandler {
  public:
    PixisHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~PixisHandler();
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
    //PixisHandler(const Pds::DetInfo& info, const EntryImage*);
    Pds::Pixis::ConfigV1  _config;
    FeatureCache&       _cache;
    int                 _iCacheIndexTemperature;
    EntryImage*         _entry;
    EntryImage*         _pentry;
    ndarray<unsigned,2> _offset;
    unsigned            _options;
  };
};

#endif
