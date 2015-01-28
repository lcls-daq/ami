#ifndef AmiQt_QtEmpty_hh
#define AmiQt_QtEmpty_hh

#include "ami/qt/QtBase.hh"

#include "qwt_plot_curve.h"
class QwtPlot;
class QColor;

namespace Ami {
  class EntryScan;
  class AbsTransform;
  namespace Qt {
    class QtEmpty : public QtBase {
    public:
      QtEmpty();
      ~QtEmpty();
    public:
      void        dump  (FILE*   ) const;
      void        attach(QwtPlot*);
      void        update()        ;
      void        xscale_update() ;
      void        yscale_update() ;
      void        set_color(unsigned);
      const AxisInfo* xinfo() const;
    private:
      QwtPlotCurve  _curve;
      AxisInfo*     _xinfo;
    };
  };
};

#endif
