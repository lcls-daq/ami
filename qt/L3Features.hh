#ifndef AmiQt_L3Features_hh
#define AmiQt_L3Features_hh

#include "ami/qt/FeatureTree.hh"

namespace Ami {
  namespace Qt {
    class L3Features : public FeatureTree {
    public:
      L3Features();
      ~L3Features();
    protected:
      bool _valid_entry(const QString&) const;
    };
  };
};

#endif
