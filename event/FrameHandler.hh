#ifndef Ami_FrameHandler_hh
#define Ami_FrameHandler_hh

#include "ami/event/EventHandler.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "ndarray/ndarray.h"

namespace Ami {
  class EntryImage;

  class FrameHandler : public EventHandler {
  public:
    FrameHandler(const Pds::DetInfo& info,
		 unsigned defColumns,
		 unsigned defRows);
    FrameHandler(const Pds::DetInfo& info,
		 const std::list<Pds::TypeId::Type>& config_types,
		 unsigned defColumns,
		 unsigned defRows);
    ~FrameHandler();
  public:
    void        rename(const char*);
  public:
    unsigned     nentries() const;
    const Entry* entry(unsigned) const;
    void         reset();
  public:
    void _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void _damaged  ();
  protected:
    void _load_pedestals();
  protected:
    FrameHandler(const Pds::DetInfo& info, const EntryImage*);
    EntryImage* _entry;
    unsigned    _defColumns;
    unsigned    _defRows;
    ndarray<int,2> _pedestals;
    unsigned       _options;
  };
};

#endif
