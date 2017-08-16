#ifndef Ami_UsdUsbFexReader_hh
#define Ami_UsdUsbFexReader_hh

#include "ami/event/EventHandlerF.hh"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class UsdUsbFexHandler : public EventHandlerF {
  public:
    UsdUsbFexHandler(const Pds::DetInfo&, FeatureCache&);
    ~UsdUsbFexHandler();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    bool         used    () const { return true; }
    void         rename  (const char*);
  private:
    const static unsigned NCHAN = 4;
    const static unsigned NAME_LEN = 64;
    char      _aliases[NCHAN][NAME_LEN];
    unsigned  _alias_mask;
    bool      _use_alias(const unsigned index) const;
  };

};

#endif
