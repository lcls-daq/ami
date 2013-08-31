#ifndef Ami_DiodeFexHandler_hh
#define Ami_DiodeFexHandler_hh

#include "ami/event/EventHandler.hh"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class DiodeFexHandler : public EventHandler {
  public:
    DiodeFexHandler(const DetInfo&, FeatureCache&);
    ~DiodeFexHandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         reset   ();
    bool         used    () const { return true; }
  private:
    FeatureCache&        _cache;
    int                  _index;
  };

};

#endif
