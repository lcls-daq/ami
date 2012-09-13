#ifndef AmiQt_PostAnalysis_hh
#define AmiQt_PostAnalysis_hh

#include "ami/qt/OverlayParent.hh"

class QString;

namespace Ami {
  namespace Qt {
    class SharedData;
    class PostAnalysis : public OverlayParent {
    public:
      static PostAnalysis* instance();
    public:
      PostAnalysis();
      virtual ~PostAnalysis() {}
      virtual void plot(const QString& y,
                        DescEntry*     x,
                        SharedData*    p) =0;
    };
  };
};

#endif
