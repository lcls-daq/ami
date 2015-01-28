#ifndef AmiQt_CFPostParent_hh
#define AmiQt_CFPostParent_hh

namespace Ami {
  namespace Qt {
    class CurveFitPost;
    class CFPostParent {
    public:
      virtual ~CFPostParent() {}
    public:
      virtual void remove_curvefit_post(CurveFitPost*) =0;
    };
  };
};

#endif
