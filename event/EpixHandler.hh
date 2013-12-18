#ifndef Ami_EpixHandler_hh
#define Ami_EpixHandler_hh

#include "ami/event/EventHandler.hh"

#include "ndarray/ndarray.h"

namespace Ami {
  class EntryImage;
  class FeatureCache;
  class EpixHandler : public EventHandler {
  public:
    EpixHandler(const Pds::Src&     info, 
		FeatureCache&       cache);
    virtual ~EpixHandler();
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
    void _load_pedestals();

    FeatureCache&       _cache;
    EntryImage*         _entry;
    EntryImage*         _pentry;
    char*               _config_buffer;
    unsigned            _options;
    ndarray<unsigned,2> _pedestals;
    ndarray<unsigned,2> _offset;

    ndarray<int,1>      _feature;
  };
};

#endif
