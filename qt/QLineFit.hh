#ifndef AmiQt_QLineFit_hh
#define AmiQt_QLineFit_hh

#include <QtGui/QAction>
#include <QtGui/QMenu>

#include "ami/data/LineFit.hh"

#include <map>

#include "qwt_plot_curve.h"

class QwtPlot;

namespace Ami {
  class Entry;
  class EntryProf;
  class EntryScan;
  class LineFitEntry;

  namespace Qt {
    class QLineFitEntry {
    public:
      QLineFitEntry(Ami::LineFit::Method, QwtPlot*);
    public:
      void attach(QwtPlot*);
      void fit   (const Entry&);
    private:
      std::string   _name;
      LineFitEntry* _fit;
      QwtPlotCurve  _curve;
      double _x[2], _y[2];
    };

    class QLineFit {
    public:
      QLineFit();
      ~QLineFit();
    public:
      void show_fit(Ami::LineFit::Method,bool);
    public:
      void update_fit(const Entry&);
    public:
      void attach(QwtPlot*);
    private:
      QwtPlot* _frame;
      std::map< Ami::LineFit::Method,QLineFitEntry* > _fits;
    };

    class QLineFitAction : public QAction {
      Q_OBJECT
    public:
      QLineFitAction(QLineFit&, Ami::LineFit::Method);
      ~QLineFitAction();
    public slots:
      void show_fit();
    private:
      QLineFit& _host;
      Ami::LineFit::Method _method;
    };

    class QLineFitMenu : public QMenu, public QLineFit {
    public:
      QLineFitMenu();
      ~QLineFitMenu();
    };
  };
};

#endif
