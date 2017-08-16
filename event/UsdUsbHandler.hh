#ifndef Ami_UsdUsbReader_hh
#define Ami_UsdUsbReader_hh

#include "ami/event/EventHandlerF.hh"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class UsdUsbHandler : public EventHandlerF {
  public:
    UsdUsbHandler(const Pds::DetInfo&, FeatureCache&);
    ~UsdUsbHandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    bool         used    () const { return true; }
    void         rename  (const char*);
  };

};

#endif
