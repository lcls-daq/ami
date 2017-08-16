#ifndef Ami_Gsc16aiHandler_hh
#define Ami_Gsc16aiHandler_hh

#include "ami/event/EventHandlerF.hh"
#include "pdsdata/psddl/gsc16ai.ddl.h"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class Gsc16aiHandler : public EventHandlerF {
  public:
    Gsc16aiHandler(const Pds::DetInfo&, FeatureCache&);
    ~Gsc16aiHandler();
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
    Pds::Gsc16ai::ConfigV1 _config;
    double                 _voltsMin;
    double                 _voltsPerCount;
  };
};

#endif
