#ifndef XPPIpmSB1_hh
#define XPPIpmSB1_hh

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
  class XPPIpmSB1 : public XppBase {
  public:
    XPPIpmSB1();
    ~XPPIpmSB1();
  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    bool accept   ();

  private:

    EntryScan*     _corr23;     // 
    EntryScan*     _corr1_02;     // 
    EntryScan*     _corr1_13;     // 
    EntryScan*     _corr1c_02;     // 
    EntryScan*     _corr1c_13;     // 
    EntryScan*     _corr1_01;     // 
    EntryScan*     _corr1_23;     // 
    EntryScan*     _corr1c_01;     // 
    EntryScan*     _corr1c_23;     // 

    EntryScan*     _corrgdet1_0;     // 
    EntryScan*     _corrgdet1_1;     // 
    EntryScan*     _corrgdet1_2;     // 
    EntryScan*     _corrgdet1_3;     // 
    EntryScan*     _corrgdet1c_0;     // 
    EntryScan*     _corrgdet1c_1;     // 
    EntryScan*     _corrgdet1c_2;     // 
    EntryScan*     _corrgdet1c_3;     // 

    EntryScan*     _corr21_0;     // 
    EntryScan*     _corr21_1;     // 
    EntryScan*     _corr21_2;     // 
    EntryScan*     _corr21_3;     // 
    EntryScan*     _corr21c_0;     // 
    EntryScan*     _corr21c_1;     // 
    EntryScan*     _corr21c_2;     // 
    EntryScan*     _corr21c_3;     // 

    EntryScan*     _corruser1_0;     // 
    EntryScan*     _corruser1_1;     // 
    EntryScan*     _corruser1_2;     // 
    EntryScan*     _corruser1_3;     // 
    EntryScan*     _corruser1c_0;     // 
    EntryScan*     _corruser1c_1;     // 
    EntryScan*     _corruser1c_2;     // 
    EntryScan*     _corruser1c_3;     // 

  };
};

#endif
  
