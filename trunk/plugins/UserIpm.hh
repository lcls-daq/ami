#ifndef UserIpm_hh
#define UserIpm_hh

//
//  Example analysis plug-in module for core online monitoring.
//
#include "XppBase.hh" // Base Xpp plugin module class

 //#include "ami/data/UserModule.hh"  // Plug-in module interface definition
 //#include "pdsdata/xtc/DetInfo.hh"
 //#include "pdsdata/xtc/BldInfo.hh"

//namespace Pds { namespace CsPad { class ElementV2; } }

// //
// //  Declare other data classes we're interested in
// //
// namespace Pds {
//   class DetInfo;                       //  Detector identity
//   class BldInfo;                       //  Detector identity
//   namespace Lusi { class IpmFexV1; };
//   class BldDataIpimbV1;
// };

namespace Ami {
  class Cds;         // Plot data service
  class EntryScalar; // Strip chart plot declaration
  class EntryScan;

  //
  //  Define our particular plug-in module
  //
  class UserIpm : public XppBase {
  public:
    UserIpm();
    ~UserIpm();

    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    bool accept   ();

  private:
    //PLOTS
    EntryScalar* _useripm[4];    //
    EntryScalar* _SB34pim[5];    // 
    EntryScan*     _corr2User[4];  // 

  };
};

#endif
  
