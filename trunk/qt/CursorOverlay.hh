#ifndef AmiQt_CursorOverlay_hh
#define AmiQt_CursorOverlay_hh

#include "ami/qt/QtOverlay.hh"
#include "ami/qt/CursorOp.hh"

#include <QtCore/QString>

namespace Ami {
  class Cds;
  class EntryScalarRange;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class QtBase;
    class QtPlot;
    class CursorOverlay : public QtOverlay,
			  public CursorOp {
    public:
      CursorOverlay(OverlayParent&   parent,
                    QtPlot&          frame,
                    unsigned         channel,
                    BinMath*         input);
      CursorOverlay(OverlayParent&   parent,
                    const char*&    p);
      ~CursorOverlay();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void dump(FILE*) const;
      const QtBase* base() const;
      void setup_payload(Cds&);
      void update();
    private:
      void _attach(Cds&);
    private:
      QtPlot*            _frame;
      QString            _frame_name;
      QtBase*            _plot;
      EntryScalarRange*  _auto_range;
      int                _order;
    };
  };
};

#endif
		 
