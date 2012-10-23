#ifndef AmiQt_EnvTable_hh
#define AmiQt_EnvTable_hh

#include <QtCore/QObject>

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <list>

class QWidget;

namespace Ami {
  class AbsFilter;
  class Cds;
  class DescScalar;
  namespace Qt {
    class QtTable;
    class EnvTable : public QObject {
      Q_OBJECT
    public:
      EnvTable(QWidget*,
               const Ami::AbsFilter& filter,
               DescScalar*     desc,
               Ami::ScalarSet  set);
      EnvTable(QWidget*,const char*&);
      ~EnvTable();
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      void configure(char*& p, unsigned input, unsigned& output);
      void setup_payload(Cds&);
      void update();
    public slots:
      void remove();
    private:
      Ami::AbsFilter* _filter;
      DescScalar*     _desc;
      Ami::ScalarSet  _set;

      unsigned _output_signature;

      ConfigureRequestor _req;
      QtTable*           _plot;
    };
  };
};

#endif
		 
