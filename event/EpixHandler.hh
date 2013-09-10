#ifndef Ami_EpixHandler_hh
#define Ami_EpixHandler_hh

#include "ami/event/EventHandler.hh"
#include "ami/data/EntryImage.hh"

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "ndarray/ndarray.h"

#include <list>

namespace Pds {
  class ClockTime;
};

namespace Ami {
  namespace Epix {
    /*
    **  Assume the ASICs are arranged in a densely populated rectangular array
    */
    class ConfigT {
    public:
      enum { typeId=Pds::TypeId::Any, version=1 };
      ConfigT() {}
      ConfigT(unsigned _chip_columns, 
	      unsigned _chip_rows, 
	      unsigned _samples, 
	      unsigned _columns, 
	      unsigned _rows) :
	nchip_columns(_chip_columns), 
	nchip_rows   (_chip_rows),
	nsamples     (_samples), 
	ncolumns     (_columns),
	nrows        (_rows) {}
    public:
      uint32_t nchip_columns;
      uint32_t nchip_rows;
      uint32_t nsamples;
      uint32_t ncolumns;
      uint32_t nrows;
    };
    class DataT {
    public:
      enum { typeId=Pds::TypeId::Any, version=2 };
      DataT() {}
    public:
      uint32_t header[8];
    };
  };
};

namespace Ami {
  class Entry;

  class EpixHandler : public EventHandler {
  public:
    EpixHandler(const Pds::Src&     info);
    virtual ~EpixHandler();
  public:
    void   _configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t);
    void   _damaged  ();
  public:
    //  Number of existing entries to advertise
    unsigned     nentries() const;
    //  Advertised entries
    const Entry* entry            (unsigned) const;
    //  Cleanup existing entries
    void         reset   ();
    //  event data needs to be parsed
    bool  used() const;
  public:
    void  rename(const char*);
  private:
    void _load_pedestals(const DescImage&);

    EntryImage*   _entry;
    Epix::ConfigT _config;
    unsigned      _options;
    ndarray<unsigned,3> _pedestals;
    ndarray<unsigned,3> _offset;
  };
};

#endif
