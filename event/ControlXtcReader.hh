#ifndef Ami_ControlXtcReader_hh
#define Ami_ControlXtcReader_hh

#include "ami/event/EventHandler.hh"

#include "ami/data/FeatureCache.hh"

namespace Pds {
  class Dgram;
};

namespace Ami {

  class ControlXtcReader : public EventHandler {
  public:
    ControlXtcReader(FeatureCache&);
    ~ControlXtcReader();
  public:
    void   _configure(const void* payload);
    void   _event    (const void* payload);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         reset   ();
  private:
    FeatureCache& _cache;
    int           _index;
  };

};

#endif
