#ifndef AmiQt_PostAnalysis_hh
#define AmiQt_PostAnalysis_hh

class QString;

namespace Ami {
  class DescEntry;
  namespace Qt {
    class SharedData;
    class PostAnalysis {
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
