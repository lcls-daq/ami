#ifndef IpmSumEScan_hh
#define IpmSumEscan_hh

#include "XppBase.hh" // Base Xpp plugin module class

namespace Ami {
  class Cds;         // Plot data service
  class EntryScalar; // Strip chart plot declaration
  class EntryScan;

  //
  //  Define our particular plug-in module
  //
  class IpmSumEScan : public XppBase {
  public:
    IpmSumEScan();
    ~IpmSumEScan();
  public:  // Analysis functions
    void clear    ();         // remove entries
    void create   (Cds& cds); // create entries
    bool accept   ();

  private:
    EntryScan*     _L3E_IPM2;         // 
    EntryScan*     _L3E_IPM3;         // 
    EntryScan*     _L3E_IPM0CH0;      // 
    EntryScan*     _L3E_IPM0CH1;      // 
    EntryScan*     _L3E_IPM0CH2;      // 
    EntryScan*     _L3E_IPM0CH3;      // 
  };
};
#endif

