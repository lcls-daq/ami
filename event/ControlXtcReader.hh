#ifndef Ami_ControlXtcReader_hh
#define Ami_ControlXtcReader_hh

#include "ami/event/EventHandler.hh"

#include <vector>

namespace Pds {
  class Dgram;
};

namespace Ami {
  class FeatureCache;
  class ControlXtcReader : public EventHandler {
  public:
    ControlXtcReader(FeatureCache&);
    ~ControlXtcReader();
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  protected:
    void   _calibrate(const void* payload, const Pds::ClockTime& t) {}
    void   _configure(const void* payload, const Pds::ClockTime& t) {}
    void   _event    (const void* payload, const Pds::ClockTime& t) {}
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    void         reset   ();
  private:
    FeatureCache& _cache;
    int _index;
    std::vector<double> _values;
  };

};

#endif
