#ifndef PhaseCavEBeam_hh
#define PhaseCavEBeam_hh

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
  class PhaseCavEBeam : public XppBase {
  public:
    PhaseCavEBeam();
    ~PhaseCavEBeam();
  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    bool accept   ();

  private:
    EntryScalar*   _scan_sb2;     // 
    EntryScalar*   _scan_sb3;     // 
    EntryScalar*   _scan_gasdet;     // 
    EntryScan*     _correbgas;     // 
    EntryScan*     _ebeam_sb2;     // 
    EntryScan*     _phasecav_t12;     //
  };
};
#endif
  
