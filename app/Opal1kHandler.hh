#ifndef Ami_Opal1kHandler_hh
#define Ami_Opal1kHandler_hh

#include "ami/event/EventHandler.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Ami {
  class EntryImage;

  class Opal1kHandler : public EventHandler {
  public:
    Opal1kHandler(const Pds::DetInfo& info);
    ~Opal1kHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  private:
    void _configure(const void* payload);
    void _event    (const void* payload);
    void _damaged  ();
  private:
    Opal1kHandler(const Pds::DetInfo& info, const EntryImage*);
    EntryImage* _entry;
  };
};

#endif
