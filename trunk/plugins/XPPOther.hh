#ifndef XPPOther_hh
#define XPPOther_hh

//
//  Example analysis plug-in module for core online monitoring.
//
#include "XppBase.hh" // Base Xpp plugin module class

namespace Ami {
  class Cds;         // Plot data service
  class EntryScalar; // Strip chart plot declaration
  class EntryScan;

  //
  //  Define our particular plug-in module
  //
  class XPPOther : public XppBase {
  public:
    XPPOther();
    ~XPPOther();

  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    bool accept   ();

  private:
    const char* _fname;
    Cds*        _cds;
    Pds::ClockTime               _clk;


    EntryScan*     _correbgas;     // 
    EntryScan*     _ebeam_sb2;     // 
    EntryScan*     _corr1c_02;     // 
    EntryScan*     _corr1c_13;     // 
    EntryScan*     _corr1c_01;     // 
    EntryScan*     _corr1c_23;     // 

    EntryScalar* _cspad_temp[4];     //  
    EntryScalar* _cspad2x2_temp[4][4];     //  
    CspadTemp* _cspad_temp_conv;    

    //debug plots for IPM on SB3 + SB1 (correlation w/ SB2)
    EntryScan*     _corr2_c3[4];     // 
    EntryScan*     _corr2_c1[4];     // 
  };
};

#endif
  
