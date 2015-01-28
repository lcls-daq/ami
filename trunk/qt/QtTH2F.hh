#ifndef AmiQt_QtTH2F_hh
#define AmiQt_QtTH2F_hh

#include "ami/qt/QtBase.hh"

#include <QtCore/QObject>

#include "qwt_plot_spectrogram.h"
class QwtPlot;
class QColor;
class QwtScaleWidget;

namespace Ami {
  class EntryTH2F;
  class AbsTransform;
  namespace Qt {
    class QtTH2F : public QObject, public QtBase {
      Q_OBJECT
    public:
      QtTH2F(const QString&   title,
	     const Ami::EntryTH2F&,
	     const AbsTransform& x,
	     const AbsTransform& y,
	     const QColor&);
      ~QtTH2F();
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
