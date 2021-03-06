#ifndef Ami_CspadHandler_hh
#define Ami_CspadHandler_hh

#include "ami/event/EventHandlerF.hh"
#include "ami/event/CspadTemp.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>

namespace Ami {
  namespace CspadGeometry { 
    class Detector; 
    class ConfigCache;
  };

  class EntryImage;
  class FeatureCache;

  class CspadHandler : public EventHandlerF {
  public:
    CspadHandler(const Pds::DetInfo& info, FeatureCache&);
    ~CspadHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    const Entry* hidden_entry(unsigned) const;
    void         reset();
  public:
    void         rename(const char*);
  public:
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  private:
    void _event    (Pds::TypeId, const char*, size_t, const Pds::ClockTime&);
    void _create_entry(const CspadGeometry::ConfigCache& cfg,
                       FILE* f, FILE* s, FILE* g, FILE* rms,
                       CspadGeometry::Detector*& detector,
                       EntryImage*& entry, 
                       unsigned max_pixels); 
  protected:
    //    CspadHandler(const Pds::DetInfo& info, const EntryImage*);
    EntryImage* _entry;
    EntryImage* _unbinned_entry;
    CspadGeometry::Detector* _detector;
    CspadGeometry::Detector* _unbinned_detector;
    unsigned             _options;
    CspadTemp            _therm;
    friend class CspadQuadHandler;
  };
};

#endif
