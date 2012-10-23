#ifndef AmiQt_QtPlot_hh
#define AmiQt_QtPlot_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/QtPlotStyle.hh"

#include "ami/data/DescEntry.hh"

#include <QtCore/QString>

class QLabel;
class QwtPlot;
class QwtPlotGrid;
class QwtLegend;

namespace Ami {
  namespace Qt {
    class AxisControl;
    class QtOverlay;
    class QtPlotSelector;
    class QtPlot : public QtPWidget {
      Q_OBJECT
    public:
      explicit QtPlot(QWidget*       parent,
                      const QString& name);
      QtPlot(QWidget*       parent);
      virtual ~QtPlot();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      virtual void dump(FILE*) const=0;
    public:
      void edit_xrange(bool);
      void edit_yrange(bool);
    private:
      void _layout();
    signals:
      void redraw();
      void counts_changed(double);
    public slots:
      void save_data();
      void set_plot_title();
      void set_xaxis_title();
      void set_yaxis_title();
      void toggle_grid();
      void toggle_minor_grid();
      void xrange_change();
      void yrange_change();
      void update_counts(double);
      void query_style();
    public:
      void set_style();
    public:
      QString      _name;
      QwtPlot*     _frame;
      QLabel*      _runnum;
      QLabel*      _counts;
    private:
      AxisControl* _xrange;
      AxisControl* _yrange;
      QwtPlotGrid* _grid;
    protected:
      QtPlotStyle _style;

    public:
      static const QStringList& names (Ami::DescEntry::Type);
      static QtPlot*            lookup(const QString&);
      static void               select(QtPlotSelector*);
    private:
      static void               _remove(QtPlot*);
    protected:
      void setPlotType      (Ami::DescEntry::Type);
      void mousePressEvent  (QMouseEvent* e);
      Ami::DescEntry::Type _type;
    public:
      void add_overlay(QtOverlay*);
    private:
      std::list<QtOverlay*> _ovls;
    };
  };
};

#endif
		 
