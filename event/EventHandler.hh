#ifndef Ami_EventHandler_hh
#define Ami_EventHandler_hh

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include <list>

namespace Pds {
  class ClockTime;
};

namespace Ami {
  class Entry;

  /**
   *  A base class for generating plottable data (Entry objects) from received
   *  XTC data.  Each EventHandler instanciation will associate itself with
   *  a particular data source (detector).  The Entry object instanciations will
   *  correspond to the data type {scalar, waveform, image} and size as
   *  indicated in the detector configuration object.
   *
   *  Note that this object does not own the Entry objects it creates.  Ownership
   *  is taken by another object.
   */
  class EventHandler {
  public:
    /**
     *   Constructor for one configuration data type and one event-data type
     *   param[in]  info           Data source 
     *   param[in]  data_type      Event data type
     *   param[in]  config_type    Configuration data type
     */
    EventHandler(const Pds::Src&     info,
		 Pds::TypeId::Type   data_type,
		 Pds::TypeId::Type   config_type);
    /**
     *   Constructor for one configuration data type and a list of event-data types
     *   param[in]  info           Data source 
     *   param[in]  data_type      List of event data types
     *   param[in]  config_type    Configuration data type
     */
    EventHandler(const Pds::Src&     info,
		 const std::list<Pds::TypeId::Type>& data_type,
		 Pds::TypeId::Type   config_type);
    /**
     *   Constructor for a list of configuration data types and one event-data type
     *   param[in]  info           Data source 
     *   param[in]  data_type      Event data type
     *   param[in]  config_type    List of configuration data types
     */
    EventHandler(const Pds::Src&     info,
		 Pds::TypeId::Type   data_type,
		 const std::list<Pds::TypeId::Type>& config_type);
    /**
     *   Constructor for a list of configuration data types and event-data types
     *   param[in]  info           Data source 
     *   param[in]  data_type      List of event data types
     *   param[in]  config_type    List of configuration data types
     */
    EventHandler(const Pds::Src&     info,
		 const std::list<Pds::TypeId::Type>& data_type,
		 const std::list<Pds::TypeId::Type>& config_type);
    virtual ~EventHandler();
  public:
    /**
     *   Handle configuration data.  This function should create/reset its Entry
     *   objects here.
     */
    virtual void   _configure(Pds::TypeId, 
			      const void* payload, const Pds::ClockTime& t) = 0;
    /**
     *   Handle configuration data on a BeginCalib transition.  The detector 
     *   configuration may have changed.
     */
    virtual void   _calibrate(Pds::TypeId, 
			      const void* payload, const Pds::ClockTime& t) = 0;
    /**
     *   Handle event data.  The Entry objects should be filled and their valid
     *   time updated.
     */
    virtual void   _event    (Pds::TypeId,
                              const void* payload, const Pds::ClockTime& t) = 0;
    /**
     *   The data was not valid for this event
     */
    virtual void   _damaged  () = 0;
  public:
    ///  Number of existing Entry objects to advertise
    virtual unsigned     nentries() const = 0;
    ///  Accessor to an advertised Entry object
    virtual const Entry* entry            (unsigned) const = 0;
    ///  Additional set of entries to advertised (not implemented)
    virtual const Entry* hidden_entry     (unsigned) const { return 0; }
    ///  Stop referencing previously created Entry objects
    virtual void         reset   () = 0;
    //  Event data needs to be parsed
    virtual bool  used() const;
  public:
    const Pds::Src&     info() const { return _info; }
    const Pds::TypeId::Type&  data_type() const { return _data_type.front(); }
    const std::list<Pds::TypeId::Type>& data_types() const { return _data_type; }
    const Pds::TypeId::Type&  config_type() const { return _config_type.front(); }
    const std::list<Pds::TypeId::Type>& config_types() const { return _config_type; }
  public:
    static void enable_full_resolution(bool);
  protected:
    bool _full_resolution() const;
  private:
    Pds::Src                     _info;
    std::list<Pds::TypeId::Type> _data_type;
    std::list<Pds::TypeId::Type> _config_type;
  };
};

#endif
