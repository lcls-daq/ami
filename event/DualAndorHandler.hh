#ifndef Ami_DualAndorHandler_hh
#define Ami_DualAndorHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/andor3d.ddl.h"

namespace Ami {
  class EntryImage;

  class DualAndorHandler : public EventHandler {
  public:
    DualAndorHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~DualAndorHandler();
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
    //AndorHandler(const Pds::DetInfo& info, const EntryImage*);
    Pds::Andor3d::ConfigV1  _config;
    FeatureCache&       _cache;
    ndarray<unsigned,1> _iCacheIndexTemperature;
    EntryImage*         _entry;
    EntryImage*         _pentry;
    ndarray<unsigned,2> _offset;
    unsigned            _options;
  };
};

#endif
