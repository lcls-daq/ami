#ifndef Ami_FccdHandler_hh
#define Ami_FccdHandler_hh

#include "ami/event/EventHandler.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/fccd/FccdConfigV2.hh"

#include <string>

namespace Ami {
  class EntryImage;

  class FccdHandler : public EventHandler {
  public:
    FccdHandler(const Pds::DetInfo& info);
    ~FccdHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  protected:
    void _calibrate(Pds::TypeId::Type, const void* payload, const Pds::ClockTime& t);
    void _calibrate(const void* payload, const Pds::ClockTime& t) {}
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _configure(const void* payload, const Pds::ClockTime& t) {}
    void _event    (const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  private:
    void _load_pedestals();
  private:
    FccdHandler(const Pds::DetInfo& info, const EntryImage*);
    EntryImage* _entry;
    unsigned*   _pedestals;
  };
};

#endif
