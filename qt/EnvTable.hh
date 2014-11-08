#ifndef AmiQt_EnvTable_hh
#define AmiQt_EnvTable_hh

#include <QtCore/QObject>

#include "ami/qt/EnvOp.hh"

#include "ami/data/DescScalar.hh"

class QWidget;

namespace Ami {
  class Cds;
  namespace Qt {
    class QtTable;
    class EnvTable : public QObject, public EnvOp {
      Q_OBJECT
    public:
      EnvTable(QWidget*,
               const Ami::AbsFilter& filter,
               DescScalar*     desc,
               Ami::ScalarSet  set);
      EnvTable(QWidget*,const char*&);
      ~EnvTable();
    public:
      void load(const char*&);
    public:
      void setup_payload(Cds&);
      void update();
    public:
      const DescScalar& desc() const { return *static_cast<const DescScalar*>(_desc); }
    public slots:
      void remove();
    signals:
      void update_plot();
      void remove(QObject*);
    private:
      QtTable*           _plot;
    };
  };
};

#endif
		 
