#ifndef CsPadTemp_hh
#define CsPadTemp_hh

#include "ami/data/UserModule.hh"
#include "ami/event/CspadTemp.hh"

#include "pdsdata/psddl/cspad.ddl.h"

#include <string>

// Forward declarations
namespace Pds {
  namespace CsPad2x2 { class ElementV1; };
};

namespace Ami {
  class Cds;         // Plot data service
  class EntryScalar; // Strip chart plot declaration
  class NameService; // Handles DAQ Alias lookups

  //
  //  Define our particular plug-in module
  //
  class CsPadTemp : public UserModule {
  public:
    CsPadTemp(const char* name="CSPadTemps",
                 const char* short_name="CspadTemp");
    virtual ~CsPadTemp();

    // Handler functions
    void reset    (FeatureCache&);
    void clock    (const Pds::ClockTime& clk);     // timestamp of current event/configure

    void configure(const Pds::DetInfo&   src,      // configuration data callback Pds:DetInfo&
                   const Pds::TypeId&    type,
                   void*                 payload);
    void configure(const Pds::BldInfo&   src,      // configuration data callback Pds:BldInfo&
                   const Pds::TypeId&    type,
                   void*                 payload);
    void configure(const Pds::ProcInfo&  src,      // configuration data callback Pds:ProcOBInfo&
                   const Pds::TypeId&    type,
                   void*                 payload);

    void event    (const Pds::DetInfo&   src,      // event data callback for Pds::DetInfo&
                   const Pds::TypeId&    type,
                   const Pds::Damage&    dmg,
                   void*                 payload);
    void event    (const Pds::BldInfo&   src,      // event data callback for Pds::BldInfo&
                   const Pds::TypeId&    type,
                   const Pds::Damage&    dmg,
                   void*                 payload);
    void event    (const Pds::ProcInfo&  src,      // event data callback for Pds::ProcInfo&
                   const Pds::TypeId&    type,
                   const Pds::Damage&    dmg,
                   void*                 payload);

    // Analysis functions
    const char* name() const;
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    void create   (Cds& cds, const uint cspad_num, const bool is_big=false); // create Entry's for individual cspad
    void remove   (const uint cspad_num, const bool is_big=false); // remove Entry's for individual cspad
    bool accept   ();         // fill   Entry's; called after all event() callbacks

  private:
    void clear_data();
    const static uint DMG_MASK = 0xFFFFFFFF;
    const static uint NUM_QUAD = 4;
    const static uint NUM_CHAN = 4;
    const static uint NUM_CSPAD = 5;
    const static uint NUM_CSPAD2x2 = 10;
    const static uint CSPAD_TEMP_CHAN = 0;
    const static double RDIV;
    Cds*            _cds;
    Pds::ClockTime  _clk;
    
    const Pds::CsPad::ElementV2* _cspad_data[NUM_CSPAD][NUM_QUAD];
    Pds::DetInfo _cspad[NUM_CSPAD];
    bool _cspad_pres[NUM_CSPAD];
    uint _cspad_num;
    Pds::CsPad::ConfigV5 _cspad_config[NUM_CSPAD];
    EntryScalar* _cspad_temp_plots[NUM_CSPAD][NUM_QUAD];

    const Pds::CsPad2x2::ElementV1* _cspad_2x2_data[NUM_CSPAD2x2];
    Pds::DetInfo _cspad_2x2[NUM_CSPAD2x2];
    bool _cspad_2x2_pres[NUM_CSPAD2x2];
    uint _cspad_2x2_num;
    EntryScalar* _cspad_2x2_temp_plots[NUM_CSPAD2x2][NUM_CHAN];
    
    const char*  _fname;
    const char*  _sname;
    std::string _cspad_name[NUM_CSPAD];
    std::string _cspad_2x2_name[NUM_CSPAD2x2];

    CspadTemp* _cspad_temp_conv;

    NameService* _name_service;
  };
};

typedef Ami::UserModule* create_t();
typedef void destroy_t(Ami::UserModule*);

#endif
