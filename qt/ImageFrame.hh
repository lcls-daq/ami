#ifndef AmiQt_ImageFrame_hh
#define AmiQt_ImageFrame_hh

#include "ami/qt/CursorTarget.hh"
#include "ami/qt/QtImage.hh"

#include <QtGui/QWidget>

class QLabel;

#include <list>

namespace Ami {
  namespace Qt {
    class Cursors;
    class ImageColorControl;
    class ImageMarker;
    class ImageGrid;
    class QtImage;
    class ImageFrame : public QWidget,
		       public CursorTarget {
      Q_OBJECT
    public:
      ImageFrame(QWidget*, const ImageColorControl&);
      ~ImageFrame();
    public:
      void attach(QtImage*);
      void setXYScale   (int);
      void setZScale    (int);
      void autoXYScale  (bool);
    public:
      const AxisInfo* xinfo() const { return _qimage->xinfo(); }
      const AxisInfo* yinfo() const { return _qimage->yinfo(); }
      float           value(unsigned x,unsigned y) const { return _qimage->value(x,y); }
    public:
      void add_marker   (ImageMarker&);
      void remove_marker(ImageMarker&);
    public slots:
      void replot();
      void scale_changed();
      void show_grid(bool);
    protected:
      void mouseReleaseEvent(QMouseEvent* e);
      void mouseMoveEvent   (QMouseEvent* e);
      void mousePressEvent  (QMouseEvent* e);
    public:
      void set_cursor_input(Cursors* c);
      void set_grid_scale(double,double);
    private:
      const ImageColorControl& _control;
      QLabel*   _canvas;
      QtImage*  _qimage;
      int       _zshift;
      bool      _xyscale;
      Cursors*  _c;
      std::list<ImageMarker*> _markers;
      ImageGrid* _xgrid;
      ImageGrid* _ygrid;
    };
  };
};

#endif
