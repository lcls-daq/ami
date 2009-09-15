#ifndef Ami_AcqWaveformHandler_hh
#define Ami_AcqWaveformHandler_hh

#include "ami/event/EventHandler.hh"

#include "pdsdata/acqiris/ConfigV1.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class EntryWaveform;

  class AcqWaveformHandler : public EventHandler {
  public:
    AcqWaveformHandler(const Pds::DetInfo& info);
    ~AcqWaveformHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  private:
    void _configure(const void* payload);
    void _event    (const void* payload);
    void _damaged  ();
  private:
    AcqWaveformHandler(const Pds::DetInfo& info, 
		       const Pds::Acqiris::ConfigV1& config);
  private:
    Pds::Acqiris::ConfigV1 _config;
    enum { MaxEntries=32 };
    unsigned       _nentries;    
    EntryWaveform* _entry[MaxEntries];
  };
};

#endif
