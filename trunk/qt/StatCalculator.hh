#ifndef AmiQt_StatCalculator_hh
#define AmiQt_StatCalculator_hh

#include "ami/data/EntryScalar.hh"
#include <QtGui/QLabel>
#include <time.h>

namespace Ami {
  namespace Qt {
    class StatCalculator : public QLabel {
      Q_OBJECT
    public:
      StatCalculator();
      virtual ~StatCalculator();
    public:
      bool set_entry(Ami::Entry* entry);
    public:
      virtual void reset () = 0;
      virtual void update() = 0;
    public slots:
      void change(QString text);
    signals:
      void changed(QString text);
    protected:
      Ami::EntryScalar* _entry;
    };
  };
};

#endif
