#ifndef Ami_EpicsXtcReader_hh
#define Ami_EpicsXtcReader_hh

#include "ami/event/EventHandler.hh"

#include "ami/data/FeatureCache.hh"

namespace Pds {
  class Dgram;
};

namespace Ami {

  class EpicsXtcReader : public EventHandler {
  public:
    EpicsXtcReader(FeatureCache&);
    ~EpicsXtcReader();
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
