#ifndef AmiQt_SMPWarning_hh
#define AmiQt_SMPWarning_hh

#include <QtGui/QPushButton>

namespace Ami {
  namespace Qt {
    class SMPWarning : public QPushButton {
      Q_OBJECT
    public:
      SMPWarning();
      ~SMPWarning();
    public slots:
      void updateVisibility();
      void showWarning();
    };
  };
};

#endif
      
