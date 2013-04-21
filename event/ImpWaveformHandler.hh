#ifndef Ami_ImpWaveformHandler_hh
#define Ami_ImpWaveformHandler_hh

#include "ami/event/EventHandler.hh"

#include "pdsdata/imp/ConfigV1.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class EntryWaveform;
  class EntryRef;

  class ImpWaveformHandler : public EventHandler {
  public:
    ImpWaveformHandler(const Pds::DetInfo& info);
    ~ImpWaveformHandler();
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
    ImpWaveformHandler(const Pds::DetInfo& info, 
		       const Pds::Imp::ConfigV1& config);
  private:
    Pds::Imp::ConfigV1 _config;
    enum { NumberOfEntries=4 };
    unsigned       _nentries;    
    EntryWaveform* _entry[NumberOfEntries];
    EntryRef*      _ref;
    unsigned       _numberOfSamples;
  };
};

#endif
