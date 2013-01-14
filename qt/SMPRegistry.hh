#ifndef AmiQt_SMPRegistry_hh
#define AmiQt_SMPRegistry_hh

#include <QtCore/QObject>

namespace Ami {
  namespace Qt {
    class SMPRegistry : public QObject {
      Q_OBJECT
    public:
      static SMPRegistry& instance();
    public:
      void     nservers(unsigned);
      unsigned nservers();
    signals:
      void changed();
    private:
      SMPRegistry();
      ~SMPRegistry();
    private:
      unsigned _nservers;
    };
  };
};

#endif
