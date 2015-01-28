#ifndef AmiQt_QtPlotCurve_hh
#define AmiQt_QtPlotCurve_hh

#include <QtCore/QObject>
#include "qwt_plot_curve.h"

namespace Ami {
  namespace Qt {
    class QtPlotCurve : public QObject,
                        public QwtPlotCurve {
      Q_OBJECT
    public:
      QtPlotCurve(const char* name);
      ~QtPlotCurve();
    public:
      void setRawData(double* x, double* y, unsigned n);
    signals:
      void setRaw();
    public slots:
      void rawSet();
    private:
      double*  _rawx;
      double*  _rawy;
      unsigned _rawn;
    };
  };
};

#endif
