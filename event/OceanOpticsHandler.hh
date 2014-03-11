#ifndef Ami_OceanOpticsHandler_hh
#define Ami_OceanOpticsHandler_hh

#include "ami/event/EventHandler.hh"

#include "pdsdata/psddl/oceanoptics.ddl.h"
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
    void         rename(const char*);
  public:
    void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  private:
    //OceanOpticsHandler(const Pds::DetInfo& info,
    //       const Pds::OceanOptics::ConfigV1& config);
  private:
    char _configBuffer[sizeof(Pds::OceanOptics::ConfigV2)];
    int  _iConfigVer;
    enum { MaxEntries=32 };
    unsigned       _nentries;
    EntryWaveform* _entry[MaxEntries];
  };
};

#endif
