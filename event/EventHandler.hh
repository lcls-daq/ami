#ifndef Ami_EventHandler_hh
#define Ami_EventHandler_hh

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds {
  class Dgram;
  class Sequence;
};

namespace Ami {
  class Entry;

  class EventHandler {
  public:
    EventHandler(const Pds::Src&     info,
		 Pds::TypeId::Type   data_type,
		 Pds::TypeId::Type   config_type);
    virtual ~EventHandler();
  public:
    virtual void   _configure(const void* payload) = 0;
    virtual void   _event    (const void* payload) = 0;
    virtual void   _damaged  () = 0;
  public:
    virtual unsigned     nentries() const = 0;
    virtual const Entry* entry   (unsigned) const = 0;
    virtual void         reset   () = 0;
  public:
    const Pds::Src&     info() const { return _info; }
    const Pds::TypeId::Type&  data_type  () const { return _data_type; }
    const Pds::TypeId::Type&  config_type() const { return _config_type; }
  private:
    Pds::Src             _info;
    Pds::TypeId::Type    _data_type;
    Pds::TypeId::Type    _config_type;
  };
};

#endif
