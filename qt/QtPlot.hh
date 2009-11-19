#ifndef AmiQt_QtPlot_hh
#define AmiQt_QtPlot_hh

#include "ami/qt/QtPWidget.hh"

#include <QtCore/QString>

class QLabel;
class QwtPlot;

namespace Ami {
  namespace Qt {
    class AxisControl;
    class QtPlot : public QtPWidget {
      Q_OBJECT
    public:
      QtPlot(QWidget*       parent,
	     const QString& name);
      QtPlot(QWidget*       parent,
	     const char*&   p);
      ~QtPlot();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      virtual void dump(FILE*) const=0;
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
      void yrange_change();
      void update_counts(double);
    protected:
      QString      _name;
      QwtPlot*     _frame;
      QLabel*      _counts;
    private:
      AxisControl* _yrange;
    };
  };
};

#endif
		 
