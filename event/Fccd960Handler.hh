#ifndef Ami_Fccd960Handler_hh
#define Ami_Fccd960Handler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/DescImage.hh"

#include "ndarray/ndarray.h"

namespace Ami {
  class EntryImage;
  class FeatureCache;
  class Fccd960Handler : public EventHandler {
  public:
    Fccd960Handler(const Pds::Src&     info, 
                   FeatureCache&       cache);
    virtual ~Fccd960Handler();
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
    void _load_gains    ();

    FeatureCache&       _cache;
    EntryImage*         _entry;
    EntryImage*         _pentry;
    unsigned            _options;
    ndarray<unsigned,2> _pedestals;
    ndarray<unsigned,2> _offset;

    ndarray<double,2>   _gain;
    ndarray<double,2>   _no_gain;
  };
};

#endif
