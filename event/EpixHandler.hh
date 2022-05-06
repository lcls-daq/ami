#ifndef Ami_EpixHandler_hh
#define Ami_EpixHandler_hh

#include "ami/event/EventHandlerF.hh"
#include "ami/event/CspadTemp.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "ami/data/DescImage.hh"

#include "ndarray/ndarray.h"

#include <vector>

namespace EpixAmi { class ConfigCache; }

namespace Ami {
  class EntryImage;
  class EntryRef;
  class EntryWaveform;
  class FeatureCache;
  class EpixHandler : public EventHandlerF {
  public:
    EpixHandler(const Pds::DetInfo&     info,
                FeatureCache&           cache);
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
  public:
    void  rename(const char*);
  private:
    void _load_pedestals();
    void _load_gains    ();

    FeatureCache&       _cache;
    DescImage           _desc;
    EntryImage*         _entry;
    std::vector<EntryWaveform*> _ewf;
    EntryRef*           _ref;
    unsigned            _options;
    bool                _do_norm;
    ndarray<unsigned,2> _status;
    ndarray<double,3>   _pedestals;
    ndarray<double,3>   _gains;

    ndarray<int,1>      _feature;

    EpixAmi::ConfigCache* _config_cache;
  };
};

#endif
