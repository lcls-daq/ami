#ifndef AmiQt_QFit_hh
#define AmiQt_QFit_hh

#include <QtGui/QAction>
#include <QtGui/QMenu>

#include "ami/qt/QAbsFitEntry.hh"

#include "ami/data/Cdu.hh"
#include "ami/data/DescEntry.hh"

#include <map>
#include <vector>

class QwtPlot;
class QColor;

namespace Ami {
  class Entry;
  class EntryProf;
  class EntryScan;
  class FitEntry;
  namespace Qt {
    class QtBase;

    class QFit {
    public:
      QFit();
      ~QFit();
    public:
      void show_fit(unsigned,bool,const QColor&);
    public:
      void update_fit(const Entry&);
    public:
      void attach(QwtPlot*);
    private:
      QwtPlot* _frame;
      std::vector< QAbsFitEntry* > _fits;
    };

    class QFitAction : public QAction {
      Q_OBJECT
    public:
      QFitAction(QFit&, const QString&, unsigned, const QColor&);
      ~QFitAction();
    public slots:
      void show_fit();
    private:
      QFit& _host;
      unsigned _function;
      QColor _color;
    };

    class QFitMenu : public QMenu, public QFit {
    public:
      QFitMenu(const QString&, const QColor&, Ami::DescEntry::Type);
      ~QFitMenu();
    };

    class QChEntry;

    class QChFitMenu : public QMenu {
    public:
      QChFitMenu(const QString&);
      ~QChFitMenu();
    public:
      void add   (const QtBase*, bool);
      void clear ();
      void update();
      void attach(QwtPlot*);
      void setPlotType(Ami::DescEntry::Type);
    public:
      void subscribe  (QChEntry&);
      void unsubscribe(QChEntry&);
    private:
      Ami::DescEntry::Type _type;
      QwtPlot* _frame;
      std::map<const QtBase*,QFitMenu*> _fits;
      std::map<QString,QFitMenu*> _save;
      std::list<QChEntry*> _entries;
    };

    class QChEntry : public Cdu {
    public:
      QChEntry(const QtBase&, Cds&, QChFitMenu& m);
      ~QChEntry();
    public:
      void clear_payload();
    private:
      const QtBase& _o;
      QChFitMenu&   _m;
    };
  };
};

#endif
