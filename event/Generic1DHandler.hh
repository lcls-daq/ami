#ifndef Ami_Generic1DHandler_hh
#define Ami_Generic1DHandler_hh

#include "ami/event/EventHandler.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/generic1d.ddl.h"
#include <vector>

namespace Ami {
  class EntryWaveform;
  class EntryRef;

  class Generic1DHandler : public EventHandler {
  public:
    Generic1DHandler(const Pds::DetInfo& info);
    ~Generic1DHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
    void         rename(const char*);
  private:
    void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  private:
    //Generic1DHandler(const Pds::DetInfo& info, 
		       //const Pds::Generic1D::ConfigV0& config);
  private:
    char* _cbuffer;
    Pds::Generic1D::ConfigV0* _config;
    enum { NumberOfEntries=8 };
    unsigned       _nentries;    
    EntryWaveform* _entry[2*NumberOfEntries];
    std::vector<EntryRef*>      _ref;
    unsigned       _numberOfSamples;
  };
};

#endif
