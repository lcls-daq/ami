#ifndef AmiQt_FeatureRegistry_hh
#define AmiQt_FeatureRegistry_hh

#include <QtCore/QObject>

#include <QtCore/QStringList>

namespace Ami {
  namespace Qt {
    class FeatureRegistry : public QObject {
      Q_OBJECT
    public:
      static FeatureRegistry& instance();
    public:
      void               clear   ();
      void               insert  (const char* names,
				  unsigned n);
    public:
      const QStringList& names   () const;
      int                index   (const QString&) const;
    signals:
      void changed();
    private:
      FeatureRegistry();
      ~FeatureRegistry();
    private:
      QStringList _names;
    };
  };
};

#endif
