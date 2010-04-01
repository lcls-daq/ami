#ifndef Ami_TM6740Handler_hh
#define Ami_TM6740Handler_hh

#include "ami/event/EventHandler.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class EntryImage;

  class TM6740Handler : public EventHandler {
  public:
    TM6740Handler(const Pds::DetInfo& info);
    ~TM6740Handler();
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
    TM6740Handler(const Pds::DetInfo& info, const EntryImage*);
    EntryImage* _entry;
  };
};

#endif
