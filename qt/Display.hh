#ifndef AmiQt_Display_hh
#define AmiQt_Display_hh

#include <QtGui/QWidget>

#include <list>

namespace Ami {
  class AbsTransform;
  namespace Qt {
    class AxisArray;
    class AxisControl;
    class Transform;
    class QtBase;
    class Cursors;
    class PlotFrame;
    class Display : public QWidget {
      Q_OBJECT
    public:
      Display();
      ~Display();
    public:
      void add   (QtBase*);
      void reset ();
      const std::list<QtBase*> plots() const;
      const AbsTransform& xtransform() const;
      const AxisArray&    xinfo     () const;
      PlotFrame*          plot      () const;
    public slots:
      void save_image();
      void save_data();
      void save_reference();
      void update();
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
      AxisArray* _xinfo;
      AxisArray* _yinfo;
      std::list<QtBase*>  _curves;
      Cursors* _cursor_input;
    };
  };
};


#endif
