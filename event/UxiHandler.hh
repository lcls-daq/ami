#ifndef Ami_UxiHandler_hh
#define Ami_UxiHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/uxi.ddl.h"

namespace Ami {
  class EntryImage;

  class UxiHandler : public EventHandler {
  public:
    UxiHandler(const Pds::DetInfo& info, FeatureCache& cache);
    ~UxiHandler();
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
    void _load_pedestals(unsigned frames, unsigned rows, unsigned columns);
  private:
    Pds::Xtc*           _configtc;
    FeatureCache&       _cache;
    EntryImage*         _entry;
    ndarray<uint16_t,3> _pedestal;
    unsigned            _options;
  };
};

#endif
