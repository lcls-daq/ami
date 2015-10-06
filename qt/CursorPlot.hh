#ifndef AmiQt_CursorPlot_hh
#define AmiQt_CursorPlot_hh

#include "ami/qt/QtPlot.hh"
#include "ami/qt/CursorOp.hh"

#include <QtCore/QString>

namespace Ami {
  class Cds;
  class DescEntry;
  class EntryAutoRange;
  namespace Qt {
    class QtBase;
    class CursorPlot : public QtPlot,
		       public CursorOp {
      Q_OBJECT
    public:
      CursorPlot(QWidget*       parent,
		 const QString& name,
		 unsigned       channel,
		 BinMath*       desc);
      CursorPlot(QWidget*       parent,
		 const char*&   p);
      ~CursorPlot();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void setup_payload(Cds&);
      void update();
      void dump(FILE*) const;
    signals:
      void changed();
    private:
      QtBase*  _plot;
      const EntryAutoRange* _auto_range;
      bool     _retry;
    };
  };
};

#endif
		 
