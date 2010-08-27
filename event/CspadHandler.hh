#ifndef Ami_CspadHandler_hh
#define Ami_CspadHandler_hh

#include "ami/event/EventHandler.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace CspadGeometry { class Detector; };

namespace Ami {
  class EntryImage;
  class FeatureCache;

  class CspadHandler : public EventHandler {
  public:
    CspadHandler(const Pds::DetInfo& info, FeatureCache&);
    ~CspadHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  private:
    void _calibrate(const void* payload, const Pds::ClockTime& t);
    void _configure(const void* payload, const Pds::ClockTime& t);
    void _event    (const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  protected:
    CspadHandler(const Pds::DetInfo& info, const EntryImage*);
    EntryImage* _entry;
    CspadGeometry::Detector* _detector;
    FeatureCache&        _cache;
  };
};

#endif
