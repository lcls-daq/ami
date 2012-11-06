#ifndef Ami_OceanOpticsHandler_hh
#define Ami_OceanOpticsHandler_hh

#include "ami/event/EventHandler.hh"

#include "pds/config/OceanOpticsConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class EntryWaveform;
  class EntryRef;

  class OceanOpticsHandler : public EventHandler {
  public:
    OceanOpticsHandler(const Pds::DetInfo& info);
    ~OceanOpticsHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  private:
    void _calibrate(const void* payload, const Pds::ClockTime& t);
    void _configure(const void* payload, const Pds::ClockTime& t);
    void _event    (const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  private:
    OceanOpticsHandler(const Pds::DetInfo& info, 
           const OceanOpticsConfigType& config);
  private:
    OceanOpticsConfigType _config;
    enum { MaxEntries=32 };
    unsigned       _nentries;    
    EntryWaveform* _entry[MaxEntries];
  };
};

#endif
