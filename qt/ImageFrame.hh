#ifndef AmiQt_ImageFrame_hh
#define AmiQt_ImageFrame_hh

#include "ami/qt/CursorTarget.hh"
#include "ami/qt/ImageOffload.hh"
#include "ami/qt/OffloadEngine.hh"
#include "ami/qt/QtImage.hh"

#include <QtGui/QWidget>
#include <QtGui/QImage>
#include <QtGui/QPixmap>

class QLabel;

#include <list>

namespace Pds { class ClockTime; };

namespace Ami {
  class Task;

  namespace Qt {
    class AxisInfo;
    class Cursors;
    class ImageColorControl;
    class ImageInspect;
    class ImageMarker;
    class QtImage;
    class ImageFrame : public QWidget,
		       public CursorTarget,
                       public ImageOffload {
      Q_OBJECT
    public:
      ImageFrame(QWidget*, ImageColorControl&);
      ~ImageFrame();
    public:
      void attach(QtImage*);
      void setXYScale   (int);
      void setZScale    (int);
    public:
      const AxisInfo* xinfo() const;
      const AxisInfo* yinfo() const;
      float           value(unsigned x,unsigned y) const { return _engine.qimage()->value(x,y); }
      const Pds::ClockTime& time() const;
    public:
      void add_marker   (ImageMarker&);
      void remove_marker(ImageMarker&);
      ImageInspect*     inspector();
    public slots:
      void replot();
      void scale_changed();
    public:
      void render_image (QImage&);
      void render_pixmap(QImage&);
    protected:
      void mouseReleaseEvent(QMouseEvent* e);
      void mouseMoveEvent   (QMouseEvent* e);
      void mousePressEvent  (QMouseEvent* e);
    public:
      void set_cursor_input(Cursors* c);
      void track_cursor_input(Cursors* c);
      void set_grid_scale(double,double);
    private:
      OffloadEngine _engine;
      QLabel*       _canvas;
      int           _zshift;
      Cursors*      _c;
      std::list<ImageMarker*> _markers;
    };
  };
};

#endif
