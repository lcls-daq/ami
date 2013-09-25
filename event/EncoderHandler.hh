#ifndef Ami_EncoderReader_hh
#define Ami_EncoderReader_hh

#include "ami/event/EventHandlerF.hh"

namespace Pds {
  class DetInfo;
};

namespace Ami {
  class FeatureCache;

  class EncoderHandler : public EventHandlerF {
  public:
    EncoderHandler(const DetInfo&, FeatureCache&);
    ~EncoderHandler();
  public:
    void   rename(const char*);
  public:
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    unsigned     nentries() const;
    const Entry* entry   (unsigned) const;
    bool         used    () const { return true; }
  private:
    int                    _index;
  };

};

#endif
