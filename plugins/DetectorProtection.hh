#ifndef DetectorProtection_hh
#define DetectorProtection_hh

//
//  Detector Protection plug-in module for core online monitoring
//

#include "ami/data/UserModule.hh"  // Plug-in module interface definition

#include <map>

namespace Ami {
  class Cds;
  class Protector;
  class Threshold;
  class EntryScalar;
  class NameService;

  //
  //  Define our particular plug-in module
  //
  class DetectorProtection : public UserModule {
  public:
    DetectorProtection(const char* name="DetectorProtection",
                       const char* short_name="DetProtect");
    virtual ~DetectorProtection();
  public:  // Handler functions
    void reset    (FeatureCache&);
    void clock    (const Pds::ClockTime& clk);     // timestamp of current event/configure
    void configure(const Pds::DetInfo&    src,      // configuration data callback
		               const Pds::TypeId&     type,
		               void*                  payload);
    void configure(const Pds::BldInfo&    src,
                   const Pds::TypeId&     type,
                   void*                  payload);
    void configure(const Pds::ProcInfo&   src,
                   const Pds::TypeId&     type,
                   void*                  payload);
    void event    (const Pds::DetInfo&    src,      // event data callback
		               const Pds::TypeId&     type,
                   const Pds::Damage&     dmg,
		               void*                  payload);
    void event    (const Pds::BldInfo&    src,      // event data callback
                   const Pds::TypeId&     type,
                   const Pds::Damage&     dmg,
                   void*                  payload);
    void event    (const Pds::ProcInfo&   src,      // event data callback
                   const Pds::TypeId&     type,
                   const Pds::Damage&     dmg,
                   void*                  payload);
  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    bool accept  ();          // fake
    const char* name() const;

  private:
    //  Resulting Plots
    Cds* _cds;

    Pds::ClockTime _clk;
    const static uint DMG_MASK = 0xFFFFFFFF;

    std::map<unsigned, Protector*> _dets;

    const char*   _fname;
    const char*   _sname;
    bool          _alias_ready;
    NameService*  _name_service;
    Threshold*    _threshold;
    FeatureCache* _cache;
  };
};

typedef Ami::UserModule* create_m();
typedef void destroy_m(Ami::UserModule*);

#endif
