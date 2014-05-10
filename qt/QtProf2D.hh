#ifndef AmiQt_QtProf2D_hh
#define AmiQt_QtProf2D_hh

#include "ami/qt/QtBase.hh"

#include <QtCore/QObject>

#include "qwt_plot_spectrogram.h"
class QwtPlot;
class QColor;
class QwtScaleWidget;

namespace Ami {
  class EntryProf2D;
  class AbsTransform;
  namespace Qt {
    class QtProf2D : public QObject, public QtBase {
      Q_OBJECT
    public:
      QtProf2D(const QString&   title,
               const Ami::EntryProf2D&,
               const AbsTransform& x,
               const AbsTransform& y,
               const QColor&);
      ~QtProf2D();
    public:
      void        dump  (FILE*   ) const;
      void        attach(QwtPlot*);
      void        update()        ;
      void        xscale_update() ;
      void        yscale_update() ;
      const AxisInfo* xinfo() const;
      double      normalization() const;
    public slots:
      void        change_color();
    signals:
      void        color_changed();
    private:
      QwtPlotSpectrogram _curve;
      class DataCache;
      DataCache*         _z;
      QwtScaleWidget*    _colorBar;
      QwtPlot*           _plot;
    };
  };
};

#endif
