#ifndef XppBase_hh
#define XppBase_hh

#include "ami/data/UserModule.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "ami/event/CspadTemp.hh"
// #include "pdsdata/cspad/ConfigV3.hh"
// #include "pdsdata/cspad/ConfigV4.hh"
// #include "pdsdata/cspad/ConfigV5.hh"

// Forward declarations
namespace Pds {
  namespace Lusi { class IpmFexV1; };
  namespace Bld { class BldDataEBeamV0; };
  namespace Bld { class BldDataPhaseCavity; };
  namespace Bld { class BldDataFEEGasDetEnergy; };
  namespace CsPad { class ElementV2; }; 
  //namespace CsPad { class ElementHeader; };
  namespace CsPad2x2 { class ElementV1; };
  //namespace CsPad2x2 { class ElementHeader; };
};

namespace Ami {
  class Cds;         // Plot data service

  //
  //  Define our particular plug-in module
  //
  class XppBase : public UserModule {
  public:
    XppBase(const char* name="XPP-Base",
            const char* short_name="XppBase");
    virtual ~XppBase();

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
                   const Pds::Damage&    damage,
                   void*                 payload);
    void event    (const Pds::BldInfo&   src,      // event data callback for Pds::BldInfo&
                   const Pds::TypeId&    type,
                   const Pds::Damage&    damage,
                   void*                 payload);
    void event    (const Pds::ProcInfo&  src,      // event data callback for Pds::ProcInfo&
                   const Pds::TypeId&    type,
                   const Pds::Damage&    damage,
                   void*                 payload);

    // Analysis functions
    const char* name() const;
    //void clear    ();         // remove Entry's
    //void create   (Cds& cds); // create Entry's
    //bool accept   ();         // fill   Entry's; called after all event() callbacks

  protected:
    void clear_data();
    const static uint NXppIpms = 9;
    //    enum XppIpms {IPM1, IPM1C, IPM2, IPM3, 
    //		  IPMMONO, IPMDEC, IPMUSER, PIM3, PIM4};
    enum XppIpms {IPM1, IPM1C, IPMMONO, IPMDEC,
		  IPM2, IPM3, PIM3, PIM4, IPMUSER};
    const static char* ipm_names[]; // see initialization under class def
    Cds*            _cds;
    Pds::ClockTime  _clk;

    //  IPM Diode data:
    const Pds::Lusi::IpmFexV1 *_ipm_data[NXppIpms];
    // Other Detector data:
    const Pds::Bld::BldDataEBeamV0 *_ebeam_data;
    const Pds::Bld::BldDataPhaseCavity* _phasecav_data;
    const Pds::Bld::BldDataFEEGasDetEnergy* _feegas_data;
    //const Pds::CsPad::ElementHeader* _cspad_data[4];
    const Pds::CsPad::ElementV2* _cspad_data[4];
    //change until cspad140 is implemented in monitoring
    //const Pds::CsPad2x2::ElementHeader* _cspad2x2_data;
    //std::vector<const Pds::CsPad::ElementHeader*> _cspad2x2_data;
    std::vector<const Pds::CsPad2x2::ElementV1*> _cspad2x2_data;
    
  private:
    const static char* ipm_ids[]; // see initialization under class def
    const char* _fname;
    const char* _sname;
    /* hardcode this - not elegant, but easiest to read+maintain */
    Pds::Src        _ipm1,
      _ipm1c,
      _ipmmono,
      _ipmdec,
      _ipm2,
      _ipm3,
      _ipmuser,
      _pim3, 
      _pim4;
    Pds::BldInfo    _ebeam;
    Pds::BldInfo    _feegas;
    Pds::BldInfo    _phasecav;
    Pds::DetInfo    _cspad;
    std::vector<Pds::DetInfo>    _cspad2x2;
    //const Pds::CsPad::ElementV2* _frame;
    //     Pds::CsPad::ConfigV3 _config3;
    //     Pds::CsPad::ConfigV4 _config4;
    //     Pds::CsPad::ConfigV5 _config5;
    int cspad_configtype;
    CspadTemp* _cspad_temp_conv;    
  };
};

const char* Ami::XppBase::ipm_ids[] = {"NH2-SB1-IPM-01",             //ipm1
				       "NH2-SB1-IPM-02",             //ipm1c
				       "XppMon_Pim1",             //ipmmomo
				       "XppMon_Pim0",             //ipmdec
				       "XppSb2_Ipm",             //ipm2
				       "XppSb3_Ipm",             //ipm3
				       "XppSb3_Pim",             //pim3
				       "XppSb4_Pim",             //pim4
				       "XppEnds_Ipm0"};             //ipmuser

const char* Ami::XppBase::ipm_names[] = {"ipm1",
                                         "ipm1c",
                                         "ipm-mono",
                                         "ipm-dec",
                                         "ipm2",
                                         "ipm3",
                                         "pim3",
                                         "pim4",
					 "ipmuser"};

typedef Ami::UserModule* create_t();
typedef void destroy_t(Ami::UserModule*);

#endif
