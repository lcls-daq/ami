#ifndef XPPIpm_hh
#define XPPIpm_hh

//
//  Example analysis plug-in module for core online monitoring.
//
#include "XppBase.hh" // Base Xpp plugin module class

namespace Ami {
  class Cds;         // Plot data service
  class EntryScalar; // Strip chart plot declaration
  class EntryScan;
  class EntryTH1F;

  //
  //  Define our particular plug-in module
  //
  class XPPIpm : public XppBase {
  public:
    XPPIpm();
    ~XPPIpm();
  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    bool accept   ();

  private:

    EntryScalar*   _scan_sb2;     // 
    EntryScalar*   _scan_sb3;     // 
    EntryScalar*   _scan_gasdet;     // 
    EntryTH1F*     _th1f_sb2;     // 
    EntryTH1F*     _th1f_sb3;     // 
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
  };
};

#endif
  
