#ifndef Ami_JungfrauHandler_hh
#define Ami_JungfrauHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/jungfrau.ddl.h"

namespace Ami {
  class EntryImage;

  namespace Jungfrau { class ConfigCache; }

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
    void _load_offsets  ();
    void _load_gains    ();
    ndarray<double,4> _load_calib(const char* online, const char* offline,
                                  double default_val, bool* used_default=NULL) const;
  private:
    Jungfrau::ConfigCache*  _config_cache;
    FeatureCache&           _cache;
    EntryImage*             _entry;
    ndarray<double,4>       _offset;
    ndarray<double,4>       _pedestal;
    ndarray<double,4>       _gain_cor;
    unsigned                _options;
  };
};

#endif
