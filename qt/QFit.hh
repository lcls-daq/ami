#ifndef AmiQt_QFit_hh
#define AmiQt_QFit_hh

#include <QtGui/QAction>
#include <QtGui/QMenu>

#include "ami/data/Fit.hh"

#include <map>

#include "qwt_plot_curve.h"

class QwtPlot;
class QColor;

namespace Ami {
  class Entry;
  class EntryProf;
  class EntryScan;
  class FitEntry;
  namespace Qt {
    class QtBase;
    class QFitEntry {
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

    class QFit {
    public:
      QFit();
      ~QFit();
    public:
      void show_fit(Ami::Fit::Function,bool,const QColor&);
    public:
      void update_fit(const Entry&);
    public:
      void attach(QwtPlot*);
    private:
      QwtPlot* _frame;
      std::map< Ami::Fit::Function,QFitEntry* > _fits;
    };

    class QFitAction : public QAction {
      Q_OBJECT
    public:
      QFitAction(QFit&, Ami::Fit::Function, const QColor&);
      ~QFitAction();
    public slots:
      void show_fit();
    private:
      QFit& _host;
      Ami::Fit::Function _function;
      QColor _color;
    };

    class QFitMenu : public QMenu, public QFit {
    public:
      QFitMenu(const QString&);
      QFitMenu(const QString&, const QColor&);
      ~QFitMenu();
    };

    class QChFitMenu : public QMenu {
    public:
      QChFitMenu(const QString&);
      ~QChFitMenu();
    public:
      void add   (QtBase*, bool);
      void clear ();
      void update();
      void attach(QwtPlot*);
    private:
      QwtPlot* _frame;
      std::map<QtBase*,QFitMenu*> _fits;
      std::map<QString,QFitMenu*> _save;
    };
  };
};

#endif
