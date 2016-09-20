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
  namespace Epics { class EpicsPvHeader; };
  namespace Epics { class ConfigV1; };
};

namespace Ami_Epics { class PVWriter; };

namespace Ami {
  class Cds;
  class EntryScalar;
  class NameService;
  class BlackHole;

  //
  //  Define our particular plug-in module
  //
  class CspadTripper : public UserModule {
  public:
    CspadTripper(const char* name="CSPadTrip",
                 const char* short_name="CSPadTrip");
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
    int32_t _nPixelsToTrip[MaxDetsAllowed];
    int32_t _tripCountThreshold[MaxDetsAllowed];
    bool    _enableTrip[MaxDetsAllowed];

    uint32_t _config_size[MaxDetsAllowed];
    Pds::CsPad::ConfigV5* _config[MaxDetsAllowed];
    unsigned _dets[MaxDetsAllowed];
    unsigned _detsFound;

    Pds::ClockTime _clk;
    Pds::ClockTime _lastTrip;
    const static uint DMG_MASK = 0xFFFFFFFF;
    const Pds::CsPad::DataV2* _frame[MaxDetsAllowed];
    const Pds::Epics::ConfigV1* _thres_config;
    int16_t _thres_epics;
    const Pds::Epics::ConfigV1* _npixel_config;
    int16_t _npixel_epics;
    const Pds::Epics::ConfigV1* _enable_config;
    int16_t _enable_epics;

    const char*  _fname;
    const char*  _sname;
    char*  _thres_pv;
    char*  _npixel_pv;
    char*  _enable_pv;
    char*  _shutter_pv;
    NameService* _name_service;
    Ami_Epics::PVWriter* _shutter;
    //    static char _nameBuffer[128];
    BlackHole* _bh;
  };
};

typedef Ami::UserModule* create_m();
typedef void destroy_m(Ami::UserModule*);

#endif
