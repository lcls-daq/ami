//
//  A class for grabbing a rectangular region of interest (ROI).
//  The ROI is inclusively delmited by the bounding coordinates.
//
#ifndef AmiQt_ImageInspect_hh
#define AmiQt_ImageInspect_hh

#include <QtGui/QWidget>
#include "ami/qt/Cursors.hh"
#include "ami/qt/ImageMarker.hh"
#include "ami/qt/QtPWidget.hh"

class QLabel;

namespace Ami {
  namespace Qt {
    class ImageFrame;
    class QtImage;
    class ImageInspect : public QWidget,
			 public Cursors,
			 public ImageMarker {
      Q_OBJECT
    public:
      ImageInspect(ImageFrame&);
      ~ImageInspect();
    public:   // ImageMarker interface
      void draw(QImage&);
    public:  // Cursors interface
      void mousePressEvent  (double,double);
      void mouseMoveEvent   (double,double);
      void mouseReleaseEvent(double,double);
    public slots:
      void toggle();
    signals:
      void changed();
    private:
      QLabel* _canvas;
      ImageFrame& _frame;
      int _x0, _y0;     // units are source pixels
    };
  };
};

#endif
