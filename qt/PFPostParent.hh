#ifndef AmiQt_PFPostParent_hh
#define AmiQt_PFPostParent_hh

namespace Ami {
  namespace Qt {
    class PeakFitPost;
    class PFPostParent {
    public:
      virtual ~PFPostParent() {}
    public:
      virtual void remove_peakfit_post(PeakFitPost*) =0;
    };
  };
};

#endif
