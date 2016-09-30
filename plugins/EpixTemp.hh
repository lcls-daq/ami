#ifndef EpixTemp_hh
#define EpixTemp_hh

#include "ami/data/UserModule.hh"

#include <string>

// Forward declarations
namespace Pds {
  namespace Epix {
    class ElementV3;
    class Config100aV2;
  };
};

namespace Ami {
  class Cds;         // Plot data service
  class EntryScalar; // Strip chart plot declaration
  class NameService; // Handles DAQ Alias lookups

  class EpixTempCache {
  public:
    EpixTempCache();
    ~EpixTempCache();

    void parse(void *payload);

    const Pds::Epix::Config100aV2& config();
  private:
    uint  _config_size;
    char* _buffer;
  };

  //
  //  Define our particular plug-in module
  //
  class EpixTemp : public UserModule {
  public:
    EpixTemp(const char* name="EpixTemps",
             const char* short_name="EpixTemp");
    virtual ~EpixTemp();

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
    void create   (Cds& cds, const uint epix_num); // create Entry's for individual epix
    void remove   (const uint epix_num); // remove Entry's for individual epix
    bool accept   ();         // fill   Entry's; called after all event() callbacks

  private:
    void clear_data();
    const static uint DMG_MASK = 0xFFFFFFFF;
    const static uint NUM_EPIX = 10;
    const static uint NUM_CHAN = 3;
    const static double RDIV=20000;
    Cds*            _cds;
    Pds::ClockTime  _clk;
    
    const Pds::Epix::ElementV3* _epix_data[NUM_EPIX];
    Pds::DetInfo _epix[NUM_EPIX];
    bool _epix_pres[NUM_EPIX];
    uint _epix_num;
    uint _epix_config_size[NUM_EPIX];
    EpixTempCache* _epix_config[NUM_EPIX];
    EntryScalar* _epix_temp_plots[NUM_EPIX][NUM_CHAN];

    const char*  _fname;
    const char*  _sname;
    std::string _epix_name[NUM_EPIX];

    NameService* _name_service;
  };
};

typedef Ami::UserModule* create_t();
typedef void destroy_t(Ami::UserModule*);

#endif
