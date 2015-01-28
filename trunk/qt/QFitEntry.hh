#ifndef AmiQt_QFitEntry_hh
#define AmiQt_QFitEntry_hh

#include "ami/qt/QAbsFitEntry.hh"

#include "ami/data/Fit.hh"

#include "qwt_plot_curve.h"

class QwtPlot;
class QColor;

namespace Ami {
  class Entry;
  class FitEntry;
  namespace Qt {
    class QtBase;
    class QFitEntry : public QAbsFitEntry {
    public:
      QFitEntry(Ami::Fit::Function, QwtPlot*, const QColor&);
    public:
      void attach(QwtPlot*);
      void fit   (const Entry&);
    private:
      std::string   _name;
      FitEntry* _fit;
      QwtPlotCurve  _curve;
    };
  };
};

#endif
