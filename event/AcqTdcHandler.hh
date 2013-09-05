#ifndef Ami_AcqTdcHandler_hh
#define Ami_AcqTdcHandler_hh

//===================================================================
//  AcqTdcHandler
//    Generates a 1D histogram with contents equal to the hit values
//===================================================================

#include "ami/event/EventHandler.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/acqiris.ddl.h"

namespace Ami {
  class EntryRef;

  class AcqTdcHandler : public EventHandler {
  public:
    AcqTdcHandler(const Pds::DetInfo& info);
    ~AcqTdcHandler();
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  private:
    void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  private:
    AcqTdcHandler(const Pds::DetInfo& info, 
		  const Pds::Acqiris::TdcConfigV1& config);
  private:
    Pds::Acqiris::TdcConfigV1 _config;
    EntryRef*                 _entry;
  };
};

#endif
