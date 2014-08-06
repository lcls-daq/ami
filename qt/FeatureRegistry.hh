#ifndef AmiQt_FeatureRegistry_hh
#define AmiQt_FeatureRegistry_hh

#include <QtCore/QObject>

#include <QtCore/QStringList>

#include "ami/data/FeatureCache.hh"
#include "ami/service/Semaphore.hh"

#include <list>
#include <map>
#include <string>

namespace Ami {
  class DiscoveryRx;

  namespace Qt {
    class SharedData;

    class FeatureRegistry : public QObject {
      Q_OBJECT
    public:
      static FeatureRegistry& instance(Ami::ScalarSet t=Ami::PostAnalysis);
    public:
      void               insert  (const std::vector<std::string>&);
    public:
      QStringList        names   () const;
      const QStringList& help    () const;
      int                index   (const QString&) const;
      QString            validate_name(const QString&) const;
      QString            find_name    (const QString&) const;
    public:
      void               share        (const QString&,SharedData*);
      void               remove       (const QString&);
    public:
      void               force   ();
    signals:
      void changed();  // discovered a change
    public:
      FeatureRegistry();
      ~FeatureRegistry();
    protected:
      QStringList _names;
      QStringList _help;
      std::map<QString,SharedData*> _shared;
      mutable Ami::Semaphore _sem;
    };
  };
};

#endif
