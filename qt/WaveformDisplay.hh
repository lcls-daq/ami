#ifndef AmiQt_WaveformDisplay_hh
#define AmiQt_WaveformDisplay_hh

#include "ami/qt/Display.hh"
#include <QtGui/QWidget>

#include <list>

namespace Ami {
  namespace Qt {
    class AxisInfo;
    class AxisControl;
    class Transform;
    class PlotFrame;
    class WaveformDisplay : public QWidget,
			    public Display {
      Q_OBJECT
    public:
      WaveformDisplay();
      ~WaveformDisplay();
    public:
      void add   (QtBase*);
      void reset ();
      void show  (QtBase*);
      void hide  (QtBase*);
      const AbsTransform& xtransform() const;
      void update();
      bool canOverlay() const { return true; }
      QWidget* widget() { return this; }
    public:
      const std::list<QtBase*> plots() const;
      const AxisInfo&     xinfo     () const;
      PlotFrame*          plot      () const;
    public slots:
      void save_image();
      void save_data();
      void save_reference();
      void xtransform_update();
      void xrange_change();
      void yrange_change();
    signals:
      void redraw();
    private:
      PlotFrame*   _plot;
      Transform*   _xtransform;
      AxisControl* _xrange;
      AxisControl* _yrange;
      const AxisInfo* _xinfo;
      const AxisInfo* _yinfo;
      std::list<QtBase*>  _curves;
      std::list<QtBase*>  _hidden;
    };
  };
};


#endif
