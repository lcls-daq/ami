#ifndef AmiQt_MaskFrame_hh
#define AmiQt_MaskFrame_hh

#include "ami/qt/CursorTarget.hh"
#include "ami/qt/ImageOffload.hh"
#include "ami/qt/OffloadEngine.hh"

#include <QtGui/QWidget>
#include <QtGui/QImage>

class QLabel;
class QScrollArea;

#include <list>

namespace Ami {
  class ImageMask;
  namespace Qt {
    class AxisInfo;
    class Cursors;
    class ImageXYControl;
    class ImageColorControl;
    class ImageMarker;
    class MaskDisplay;
    class QtImage;
    class MaskFrame : public QWidget,
                      public CursorTarget,
                      public ImageOffload {
      Q_OBJECT
    public:
      MaskFrame(MaskDisplay*, const ImageXYControl&, const ImageColorControl&);
      ~MaskFrame();
    public:
      void attach_bkg   (QtImage&);
      void attach_mask  (ImageMask&);
      void attach_marker(ImageMarker&);
      void setXYScale   (int);
      void setZScale    (int);
    public:
      void render_image (QImage&);
      void render_pixmap(QImage&);
    public:
      const AxisInfo* xinfo() const;
      const AxisInfo* yinfo() const;
    public slots:
      void replot();
      void scale_changed();
    protected:
      void mouseReleaseEvent(QMouseEvent* e);
      void mouseMoveEvent   (QMouseEvent* e);
      void mousePressEvent  (QMouseEvent* e);
    public:
      void set_cursor_input(Cursors* c);
    signals:
      void changed();
    private:
      MaskDisplay*  _parent;
      OffloadEngine _engine;
      const ImageXYControl& _xycontrol;
      QScrollArea* _scroll_area;
      QLabel*      _canvas;
      ImageMask*   _mask;
      std::list<ImageMarker*> _markers;
      Cursors*     _c;
    };
  };
};

#endif
