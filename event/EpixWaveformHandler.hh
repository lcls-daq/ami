#ifndef Ami_EpixWaveformHandler_hh
#define Ami_EpixWaveformHandler_hh

#include "ami/event/EventHandlerF.hh"

#include "ndarray/ndarray.h"

namespace Ami {
  class EntryWaveform;
  class EntryRef;
  class FeatureCache;
  class EpixWaveformHandler : public EventHandlerF {
  public:
    EpixWaveformHandler(const Pds::Src&     info, 
			FeatureCache&       cache);
    virtual ~EpixWaveformHandler();
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
    char*               _config_buffer;
    enum { EntriesPerRef=4, MaxEntries=16 };
    unsigned            _nentries;
    unsigned            _nref;
    EntryWaveform*      _entry[MaxEntries];
    EntryRef*           _ref[MaxEntries/EntriesPerRef];
    ndarray<int,1>      _feature;
    ndarray<double,1>   _filter;
    ndarray<double,1>   _gain;
    unsigned            _first_sample;
    unsigned            _last_sample;
  };
};

#endif
