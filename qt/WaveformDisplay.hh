#ifndef AmiQt_WaveformDisplay_hh
#define AmiQt_WaveformDisplay_hh

#include "ami/qt/Display.hh"
#include "ami/data/Cdu.hh"
#include <QtGui/QWidget>

#include "ami/service/Semaphore.hh"
#include <list>

class QwtPlotCurve;
class QwtPlotGrid;
class QAction;
class QLayout;

namespace Ami {
  class DescEntry;
  namespace Qt {
    class AxisInfo;
    class AxisBins;
    class AxisControl;
    class Transform;
    class PlotFrame;
    class QChFitMenu;
    class WaveformDisplay : public QWidget,
			    public Display,
			    public Cdu {
      Q_OBJECT
    public:
      WaveformDisplay();
      ~WaveformDisplay();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void prototype(const Ami::DescEntry*);
      void add   (QtBase*, Cds&, bool);
      void reset ();
      void show  (QtBase*);
      void hide  (QtBase*);
      const AbsTransform& xtransform() const;
      void update();
      bool canOverlay() const { return true; }
      QWidget* widget() { return this; }
    public:
      void save_plots(const QString&) const;
    public:
      const std::list<QtBase*> plots() const;
      const AxisInfo&     xinfo     () const;
      PlotFrame*          plot      () const;
    public:
      void                clear_payload();
    public slots:
      void save_image();
      void save_data();
      void save_reference();
      void xtransform_update();
      void xrange_change();
      void yrange_change();
      void set_plot_title();
      void set_xaxis_title();
      void set_yaxis_title();
      void toggle_grid();
      void toggle_minor_grid();
      void set_reference();
      void show_reference();
    signals:
      void redraw();
    private:
      PlotFrame*   _plot;
      QwtPlotGrid* _grid;

      Transform*   _xtransform;
      AxisControl* _xrange;
      AxisControl* _yrange;
      AxisBins* _xbins;
      const AxisInfo* _xinfo;
      const AxisInfo* _yinfo;
      std::list<QtBase*>  _curves;
      std::list<QtBase*>  _hidden;
      mutable Ami::Semaphore _sem;

      QAction* _show_ref;
      QwtPlotCurve* _ref;

    public slots:
      void toggle_chrome();
    signals:
      void set_chrome_visible(bool);
    private:
      bool     _chrome_is_visible;
      QAction* _chrome_action;
      QLayout* _chrome_layout;

    private:
      QChFitMenu* _fit;
    };
  };
};


#endif
