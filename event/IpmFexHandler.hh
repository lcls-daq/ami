#ifndef Ami_IpmFexHandler_hh
#define Ami_IpmFexHandler_hh

#include "ami/event/EventHandler.hh"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class IpmFexHandler : public EventHandler {
  public:
    IpmFexHandler(const DetInfo&, FeatureCache&);
    ~IpmFexHandler();
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
    void         rename  (const char*);
  private:
    FeatureCache&        _cache;
    enum { NChannels=7 };
    int                  _index[NChannels];
  };

};

#endif
