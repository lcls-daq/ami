#ifndef AmiQt_QLineFitEntry_hh
#define AmiQt_QLineFitEntry_hh

#include <QtGui/QAction>
#include <QtGui/QMenu>

#include "ami/qt/QAbsFitEntry.hh"
#include "ami/data/LineFit.hh"

#include <map>

#include "qwt_plot_curve.h"

class QwtPlot;

namespace Ami {
  class Entry;
  class LineFitEntry;

  namespace Qt {
    class QLineFitEntry : public QAbsFitEntry {
    public:
      QLineFitEntry(Ami::LineFit::Method, QwtPlot*, const QColor& c);
    public:
      void attach(QwtPlot*);
      void fit   (const Entry&);
    private:
      std::string   _name;
      LineFitEntry* _fit;
      QwtPlotCurve  _curve;
      double _x[2], _y[2];
    };
  };
};

#endif
