#ifndef AmiQt_ImageFrame_hh
#define AmiQt_ImageFrame_hh

#include "ami/qt/CursorTarget.hh"

#include <QtGui/QWidget>

class QLabel;

#include <list>

namespace Ami {
  namespace Qt {
    class Cursors;
    class ImageColorControl;
    class ImageMarker;
    class QtImage;
    class ImageFrame : public QWidget,
		       public CursorTarget {
      Q_OBJECT
    public:
      ImageFrame(QWidget*, const ImageColorControl&);
      ~ImageFrame();
    public:
      void attach(QtImage&);
      void setXYScale   (int);
      void setZScale    (int);
      void autoXYScale  (bool);
    public:
      void add_marker   (ImageMarker&);
      void remove_marker(ImageMarker&);
    public slots:
      void replot();
      void scale_changed();
    protected:
      void mousePressEvent(QMouseEvent* e);
    public:
      void set_cursor_input(Cursors* c);
    private:
      const ImageColorControl& _control;
      QLabel*   _canvas;
      QtImage*  _qimage;
      int       _zshift;
      bool      _xyscale;
      Cursors*  _c;
      std::list<ImageMarker*> _markers;
    };
  };
};

#endif
