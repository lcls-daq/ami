#ifndef Ami_EpixArrayHandler_hh
#define Ami_EpixArrayHandler_hh

#include "ami/event/EventHandlerF.hh"
#include "ami/data/EntryImage.hh"

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "ndarray/ndarray.h"

#include <list>

namespace Pds {
  class ClockTime;
};

namespace EpixArray { class ConfigCache; }

namespace Ami {
  class Entry;
  class FeatureCache;

  class EpixArrayHandler : public EventHandlerF {
  public:
    EpixArrayHandler(const Pds::Src& info,
                     FeatureCache&   cache);
    virtual ~EpixArrayHandler();
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    //  Number of existing entries to advertise
    unsigned     nentries() const;
    //  Advertised entries
    const Entry* entry            (unsigned) const;
    //  Cleanup existing entries
    void         reset   ();
    //  event data needs to be parsed
    bool  used() const;
  public:
    void  rename(const char*);
  private:
    void _load_pedestals(const DescImage&);

    DescImage     _desc;
    EntryImage*   _entry;

    ndarray<int,1> _feature;
    EpixArray::ConfigCache* _config_cache;

    unsigned      _options;
    ndarray<unsigned,3> _pedestals;
    ndarray<unsigned,3> _offset;
  };
};

#endif
