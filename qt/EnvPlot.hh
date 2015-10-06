#ifndef AmiQt_EnvPlot_hh
#define AmiQt_EnvPlot_hh

#include "ami/qt/QtPlot.hh"
#include "ami/qt/EnvOp.hh"

#include <QtCore/QString>

#include <list>

namespace Ami {
  class Cds;
  class EntryAutoRange;
  namespace Qt {
    class QtBase;
    class SharedData;
    class EnvPlot : public QtPlot, public EnvOp {
      Q_OBJECT
    public:
      EnvPlot(QWidget*,
	      const QString&  name,
	      const Ami::AbsFilter& filter,
	      DescEntry*      desc,
              Ami::ScalarSet  set,
              SharedData*     shared=0);
      EnvPlot(QWidget*,
	      const QString&  name,
	      const Ami::AbsFilter& filter,
	      DescEntry*      desc,
              AbsOperator*    op,
              unsigned        channel,
              SharedData*     shared=0);
      EnvPlot(QWidget*,const char*&);
      ~EnvPlot();
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      void setup_payload(Cds&);
      void update();
      void dump(FILE*) const;
    private:
      bool _forceRequest() const;
    signals:
      void changed();
    private:
      QtBase*  _plot;

      const EntryAutoRange*    _auto_range;
      bool               _retry;

      SharedData*        _shared;
    };
  };
};

#endif
		 
