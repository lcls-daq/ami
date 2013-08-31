#ifndef Ami_SharedPimHandler_hh
#define Ami_SharedPimHandler_hh

#include "ami/event/TM6740Handler.hh"
#include "pdsdata/xtc/BldInfo.hh"

namespace Ami {
  class SharedPimHandler : public EventHandler {
  public:
    SharedPimHandler(const Pds::BldInfo& info);
  public:
    void   _damaged  ();
  public:
    void   _configure(Pds::TypeId, 
		      const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, 
		      const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, 
		      const void* payload, const Pds::ClockTime& t);
  public:
    unsigned     nentries() const;
    const Entry* entry            (unsigned) const;
    void         reset   ();
    bool         used    () const { return true; }
  private:
    TM6740Handler _handler;
  };
};

#endif
