 #ifndef AmiQt_QtChart_hh
#define AmiQt_QtChart_hh

#include "ami/qt/QtBase.hh"

#include "ami/qt/QtPlotCurve.hh"

#include "ami/data/EntryScalar.hh"

#include <QtGui/QPen>

class QwtPlot;
class QColor;

namespace Ami {
  class EntryChart;
  namespace Qt {
    class QtChart : public QtBase {
      enum { DefaultPenSize=1 };
    public:
      QtChart(const QString&   title,
	      const Ami::EntryScalar&,
	      const QColor&,
              int          pen_size = DefaultPenSize);
      virtual ~QtChart();
    public:
      void        dump  (FILE*   ) const;
      void        attach(QwtPlot*);
      void        update()        ;
      void        xscale_update() ;
      void        yscale_update() ;
      void        set_color(unsigned);
      const AxisInfo* xinfo() const;
    private:
      EntryScalar&     _cache;
      unsigned         _n;  // max # of pts
      unsigned         _skip;
      unsigned         _current;
      QtPlotCurve      _curve;
      unsigned         _pts; // accumulate # of pts
      double*          _x;
      double*          _y;
      AxisInfo*     _xinfo;
    };
  };
};

#endif
