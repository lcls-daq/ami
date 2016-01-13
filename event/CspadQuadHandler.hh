#ifndef Ami_CspadQuadHandler_hh
#define Ami_CspadQuadHandler_hh

#include "ami/event/EventHandlerF.hh"
#include "ami/event/CspadTemp.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>

namespace Ami {
  class CspadHandler;

  class CspadQuadHandler : public EventHandlerF {
  public:
    CspadQuadHandler(const Pds::DetInfo& info, FeatureCache&);
    ~CspadQuadHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    const Entry* hidden_entry(unsigned) const;
    void         reset();
  public:
    void         rename(const char*);
  public:
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  private:
    void _event    (Pds::TypeId, const char*, size_t, const Pds::ClockTime&);
  protected:
    std::vector<CspadHandler*> _handlers;
    Pds::TypeId _cfgType;
    char*       _cfgPayload;
  };
};

#endif
