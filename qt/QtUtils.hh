#ifndef AmiQt_QtUtils_hh
#define AmiQt_QtUtils_hh

class QLayout;

namespace Ami {
  namespace Qt {
    class QtUtils {
    public:
      static void setChildrenVisible(QLayout*, bool);
    };
  };
};

#endif    
