#ifndef Ami_BldSpectrometerHandler_hh
#define Ami_BldSpectrometerHandler_hh

#include "ami/event/EventHandlerF.hh"
#include "pdsdata/psddl/bld.ddl.h"
#include "pdsdata/xtc/BldInfo.hh"

namespace Ami {
  class EntryWaveform;
  class EntryRef;

  class BldSpectrometerHandler : public EventHandlerF {
  public:
    BldSpectrometerHandler(const Pds::BldInfo& info, FeatureCache&);
    ~BldSpectrometerHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  public:
    void         rename(const char*);
  private:
    void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  private:
    enum { MaxEntries=2 };
    unsigned       _nentries;    
    EntryWaveform* _entry[MaxEntries];
    unsigned       _npeaks;
    int            _index;
  };
};

#endif
