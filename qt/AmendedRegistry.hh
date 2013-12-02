#ifndef AmiQt_AmendedRegistry_hh
#define AmiQt_AmendedRegistry_hh

#include "ami/qt/FeatureRegistry.hh"

#include <QtCore/QStringList>

namespace Ami {
  namespace Qt {
    class AmendedRegistry : public FeatureRegistry {
      Q_OBJECT
    public:
      AmendedRegistry(FeatureRegistry&, const QStringList&);
    public:
      void translate(char*) const;
    public slots:
      void change();
    private:
      FeatureRegistry& _base;
      QStringList      _additions;
    };
  };
};

#endif
