#ifndef CspadTripper_hh
#define CspadTripper_hh

//
//  Noise analysis plug-in module for core online monitoring.
//

#include "ami/data/UserModule.hh"  // Plug-in module interface definition

static const unsigned MaxDetsAllowed = 3;

namespace Pds {
  namespace CsPad { class DataV2; };
  namespace CsPad { class ConfigV5; };
  namespace CsPad2x2 { class ElementV1; };
};

namespace Ami {
  class Cds;
  class EntryScalar;
  class NameService;

  //
  //  Define our particular plug-in module
  //
  class CspadTripper : public UserModule {
  public:
    CspadTripper(const char* name="CSPadTrip",
                 const char* short_name="CspadTrip");
    virtual ~CspadTripper();
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
    bool accept  ();         // fake
    const char* name() const;
    void analyzeDetector ();         // fill   Entry's; called from event

  private:
    //  Resulting Plots
    Cds* _cds;
    EntryScalar* _result[MaxDetsAllowed];

    unsigned _nevt;
    unsigned _nPixelsToTrip[MaxDetsAllowed];
    unsigned _tripCountThreshold[MaxDetsAllowed];

    const Pds::CsPad::ConfigV5* _config[MaxDetsAllowed];
    unsigned _dets[MaxDetsAllowed];
    unsigned _detsFound;
    unsigned _currentDet;

    Pds::ClockTime _clk;
    Pds::ClockTime _lastTrip;
    const static uint DMG_MASK = 0xFFFFFFFF;
    const Pds::CsPad::DataV2* _frame;
    const Pds::CsPad2x2::ElementV1* _frame_140k;

    const char*  _fname;
    const char*  _sname;
    NameService* _name_service;
    //    static char _nameBuffer[128];
  };
};

typedef Ami::UserModule* create_m();
typedef void destroy_m(Ami::UserModule*);

#endif
