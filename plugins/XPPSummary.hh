#ifndef XPPSummary_hh
#define XPPSummary_hh

//
//  Example analysis plug-in module for core online monitoring.
//

#include "ami/data/UserModule.hh"  // Plug-in module interface definition
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "ami/event/CspadTemp.hh"
#include "pdsdata/psddl/cspad.ddl.h"

namespace Pds { namespace CsPad { class ElementV2; } }

//
//  Declare other data classes we're interested in
//
namespace Pds {
  class DetInfo;                       //  Detector identity
  class BldInfo;                       //  Detector identity
  namespace Lusi { class IpmFexV1; };
  namespace Bld {
    class BldDataIpimbV1;
    class BldDataEBeamV0;
    class BldDataPhaseCavity;
    class BldDataFEEGasDetEnergy;
  };
  namespace CsPad { class ElementV2; };
  namespace CsPad2x2 { class ElementV1; };
};

namespace Ami {
  class Cds;         // Plot data service
  class EntryScalar; // Strip chart plot declaration
  class EntryScan;
  class EntryTH1F;

  //
  //  Define our particular plug-in module
  //
  class XPPSummary : public UserModule {
  public:
    XPPSummary();
    ~XPPSummary();
  public:  // Handler functions
    const char* name() const;
    void reset    (FeatureCache&);
    void clock    (const Pds::ClockTime& clk);     // timestamp of current event/configure
    void configure(const Pds::DetInfo&   src,      // configuration data callback const Pds::TypeId&    type,
                   const Pds::TypeId&    type,
                   void*                 payload) { _configure(src,type,payload); }
    void configure(const Pds::BldInfo&   src,      // configuration data callback const Pds::TypeId&    type,
                   const Pds::TypeId&    type,
                   void*                 payload) { _configure(src,type,payload); }
    void configure(const Pds::ProcInfo&  src,      // configuration data callback const Pds::TypeId&    type,
                   const Pds::TypeId&    type,
                   void*                 payload) { _configure(src,type,payload); }
    void event    (const Pds::DetInfo&   src,      // event data callback
                   const Pds::TypeId&    type,
                   void*                 payload) { _event(src,type,payload); }
    void event    (const Pds::BldInfo&   src,      // event data callback
                   const Pds::TypeId&    type,
                   void*                 payload) { _event(src,type,payload); }
    void event    (const Pds::ProcInfo&  src,      // event data callback
                   const Pds::TypeId&    type,
                   void*                 payload) { _event(src,type,payload); }
  private:
    void _configure(const Pds::Src&       src,      // configuration data callback const Pds::TypeId&    type,
                    const Pds::TypeId&    type,
                    void*                 payload);
    void _event    (const Pds::Src&       src,      // event data callback
                    const Pds::TypeId&    type,
                    void*                 payload);
  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    bool accept   ();
    bool analyze   ();

  private:
    const char* _fname;
    Cds*        _cds;
    Pds::ClockTime               _clk;
    /* hardcode this - not elegant, but easiest to read+maintain */
//     Pds::DetInfo    _mon1DAQ;
//     Pds::DetInfo    _mon2DAQ;
//     Pds::DetInfo    _sb2DAQ;
//     Pds::DetInfo    _sb3DAQ;
//     Pds::DetInfo    _sb4DAQ;
//     Pds::DetInfo    _ipmuserDAQ;
    Pds::DetInfo    _cspad;
    std::vector<Pds::DetInfo>    _cspad2x2;

//     Pds::BldInfo    _mon1;
//     Pds::BldInfo    _mon2;
//     Pds::BldInfo    _sb2;
//     Pds::BldInfo    _sb3;
//     Pds::BldInfo    _sb3p;
//     Pds::BldInfo    _sb4;
//     Pds::BldInfo    _ipmuser;
//     Pds::BldInfo    _sb1;

    Pds::Src    _mon1;
    Pds::Src    _mon2;
    Pds::Src    _sb2;
    Pds::Src    _sb3;
    Pds::Src    _sb3p;
    Pds::Src    _sb4p;
    Pds::Src    _ipmuser;
    Pds::Src    _sb1;
    Pds::Src    _sb1c;

    Pds::BldInfo    _ebeam;
    Pds::BldInfo    _phasecav;
    Pds::BldInfo    _feegas;

    const Pds::Lusi::IpmFexV1* _sb2_data;    //  The Diode data
    const Pds::Lusi::IpmFexV1* _sb3_data;
    const Pds::Lusi::IpmFexV1* _sb3p_data;
    const Pds::Lusi::IpmFexV1* _sb4p_data;
    const Pds::Lusi::IpmFexV1* _ipmuser_data;
    const Pds::Lusi::IpmFexV1* _mon1_data;
    const Pds::Lusi::IpmFexV1* _mon2_data;
    const Pds::Lusi::IpmFexV1* _sb1_data;
    const Pds::Lusi::IpmFexV1* _sb1c_data;
    const Pds::Bld::BldDataEBeamV0* _ebeam_data;
    const Pds::Bld::BldDataPhaseCavity* _phasecav_data;
    const Pds::Bld::BldDataFEEGasDetEnergy* _feegas_data;
    const Pds::CsPad::ElementV2* _cspad_data[4];
    //change until cspad140 is implemented in monitoring
    //const Pds::CsPad2x2::ElementHeader* _cspad2x2_data;
    std::vector<const Pds::CsPad2x2::ElementV1*> _cspad2x2_data;

    EntryScalar*   _scan_sb2;     // 
    EntryScalar*   _scan_sb3;     // 
    EntryScalar*   _scan_gasdet;     // 
    EntryTH1F*     _th1f_sb2;     // 
    EntryTH1F*     _th1f_sb3;     // 
    EntryScan*     _correbgas;     // 
    EntryScan*     _ebeam_sb2;     // 
    EntryScan*     _phasecav_t12;     //
    EntryScan*     _corr23;     // 
    EntryScan*     _corr1_02;     // 
    EntryScan*     _corr1_13;     // 
    EntryScan*     _corr2_02;     // 
    EntryScan*     _corr2_13;     // 
    EntryScan*     _corr3_02;     // 
    EntryScan*     _corr3_13;     // 
    EntryScan*     _corr1_01;     // 
    EntryScan*     _corr1_23;     // 
    EntryScan*     _corr2_01;     // 
    EntryScan*     _corr2_23;     // 
    EntryScan*     _corr3_01;     // 
    EntryScan*     _corr3_23;     // 
    EntryScan*     _corr1c_02;     // 
    EntryScan*     _corr1c_13;     // 
    EntryScan*     _corr1c_01;     // 
    EntryScan*     _corr1c_23;     // 
    EntryScalar* _useripm[4];    //
    EntryScalar* _SB34pim[5];    // 
    EntryScalar* _mondec[4];    // 
    EntryScalar* _mondio[2];    // 
    EntryScan*     _corr2User[4];  // 
    EntryScan*     _corrMonDecDio[2];  // 

    EntryScalar* _cspad_temp[4];     //  
    EntryScalar* _cspad2x2_temp[4][4];     //  
    CspadTemp* _cspad_temp_conv;    

    //debug plots for IPM on SB3 + SB1 (correlation w/ SB2)
    EntryScan*     _corr2_c3[4];     // 
    EntryScan*     _corr2_c1[4];     // 

    const Pds::CsPad::ElementV2* _frame;
    //    Pds::CsPad::ConfigV3 _config;
    Pds::CsPad::ConfigV5 _config;   // dflath 2013-06-05
  };
};

typedef Ami::UserModule* create_t();
typedef void destroy_t(Ami::UserModule*);

#endif
  
