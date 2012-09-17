#ifndef AmiQt_MaskFrame_hh
#define AmiQt_MaskFrame_hh

#include "ami/qt/CursorTarget.hh"

#include <QtGui/QWidget>

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
    class QtImage;
    class MaskFrame : public QWidget,
                      public CursorTarget {
      Q_OBJECT
    public:
      MaskFrame(QWidget*, const ImageXYControl&, const ImageColorControl&);
      ~MaskFrame();
    public:
      void attach_bkg   (QtImage&);
      void attach_mask  (ImageMask&);
      void attach_marker(ImageMarker&);
      void setXYScale   (int);
      void setZScale    (int);
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
      const ImageXYControl& _xycontrol;
      const ImageColorControl& _control;
      QScrollArea* _scroll_area;
      QLabel*      _canvas;
      QtImage*     _qimage;
      ImageMask*   _mask;
      std::list<ImageMarker*> _markers;
      Cursors*     _c;
    };
  };
};

#endif
