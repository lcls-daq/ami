#ifndef AmiQt_RecvCalculator_hh
#define AmiQt_RecvCalculator_hh

#include <QtGui/QLabel>

namespace Ami {
  class ConnectionManager;
  namespace Qt {
    class RecvCalculator : public QLabel {
      Q_OBJECT
    public:
      RecvCalculator(ConnectionManager&);
      ~RecvCalculator();
    public:
      void update();
    public slots:
      void change(QString text);
    signals:
      void changed(QString text);
    private:
      ConnectionManager& _m;
    };
  };
};

#endif
