#ifndef Ami_CspadMiniHandler_hh
#define Ami_CspadMiniHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/event/CspadTemp.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>

namespace Ami {
  namespace CspadMiniGeometry { 
    class ConfigCache;
    class Detector; 
  };

  class EntryImage;
  class FeatureCache;

  class CspadMiniHandler : public EventHandler {
  public:
    CspadMiniHandler(const Pds::DetInfo& info, FeatureCache&, 
                     unsigned max_pixels=600);
    ~CspadMiniHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    const Entry* hidden_entry(unsigned) const;
    void         reset();
    bool         used() const;
  public:
    void         rename(const char*);
  public:
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  public:
    void _create_entry(const CspadMiniGeometry::ConfigCache& cfg,
                       FILE* f, FILE* s, FILE* g, FILE* rms, FILE* gm,
                       CspadMiniGeometry::Detector*& detector,
                       EntryImage*& entry, 
                       unsigned max_pixels); 
  protected:
    EntryImage* _entry;
    CspadMiniGeometry::Detector* _detector;
    FeatureCache&        _cache;
    unsigned             _max_pixels;
    unsigned             _options;
    CspadTemp            _therm;
  };
};

#endif
