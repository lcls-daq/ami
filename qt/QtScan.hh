#ifndef AmiQt_QtScan_hh
#define AmiQt_QtScan_hh

#include "ami/qt/QtBase.hh"

#include "qwt_plot_curve.h"
#include "qwt_symbol.h"
class QwtPlot;
class QColor;

namespace Ami {
  class EntryScan;
  class AbsTransform;
  namespace Qt {
    class QtScan : public QtBase {
      enum { DefaultSymbolSize=5 };
      enum { DefaultSymbolStyle=QwtSymbol::Diamond };
    public:
      QtScan(const QString&   title,
	     const Ami::EntryScan&,
	     const AbsTransform& x,
	     const AbsTransform& y,
	     const QColor&,
             int symbol_size=DefaultSymbolSize,
             int symbol_style=DefaultSymbolStyle);
      ~QtScan();
    public:
      void        dump  (FILE*   ) const;
      void        attach(QwtPlot*);
      void        update()        ;
      void        xscale_update() ;
      void        yscale_update() ;
      void        set_color(unsigned);
      const AxisInfo* xinfo() const;
    private:
      const AbsTransform&     _xscale;
      const AbsTransform&     _yscale;
      QwtPlotCurve     _curve;
      double*          _x;
      double*          _y;
      double*          _xa;
      AxisInfo*     _xinfo;
    };
  };
};

#endif
