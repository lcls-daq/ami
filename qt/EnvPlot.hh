#ifndef AmiQt_EnvPlot_hh
#define AmiQt_EnvPlot_hh

#include "ami/qt/QtPlot.hh"
#include <QtCore/QString>

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

#include <list>

namespace Ami {
  class AbsFilter;
  class Cds;
  class DescEntry;
  class EntryAutoRange;
  namespace Qt {
    class QtBase;
    class SharedData;
    class EnvPlot : public QtPlot {
      Q_OBJECT
    public:
      EnvPlot(QWidget*,
	      const QString&  name,
	      const Ami::AbsFilter& filter,
	      DescEntry*      desc,
              Ami::ScalarSet  set,
              SharedData*     shared=0);
      EnvPlot(QWidget*,const char*&);
      ~EnvPlot();
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      void configure(char*& p, unsigned input, unsigned& output);
      void setup_payload(Cds&);
      void update();
      void dump(FILE*) const;
    signals:
      void changed();
    private:
      Ami::AbsFilter* _filter;
      DescEntry* _desc;
      Ami::ScalarSet _set;

      unsigned _output_signature;

      QtBase*  _plot;
      ConfigureRequestor _req;

      EntryAutoRange*    _auto_range;
      bool               _retry;

      SharedData*        _shared;
    };
  };
};

#endif
		 
