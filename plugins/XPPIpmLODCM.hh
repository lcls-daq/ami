#ifndef XPPIpmLODCM_hh
#define XPPIpmLODCM_hh

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
  class XPPIpmLODCM : public XppBase {
  public:
    XPPIpmLODCM();
    ~XPPIpmLODCM();

  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    bool accept   ();

  private:
    EntryScalar* _mondec[4];    // 
    EntryScalar* _mondio[2];    // 
    EntryScan*     _corrMonDecDio[2];  // 

  };
};

#endif
  
