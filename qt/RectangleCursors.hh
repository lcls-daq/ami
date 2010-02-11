#ifndef AmiQt_RectangleCursors_hh
#define AmiQt_RectangleCursors_hh

#include <QtGui/QWidget>
#include "ami/qt/Cursors.hh"
#include "ami/qt/ImageMarker.hh"

class QLineEdit;

namespace Ami {
  namespace Qt {
    class ImageFrame;
    class RectangleCursors : public QWidget,
			     public Cursors,
			     public ImageMarker {
      Q_OBJECT
    public:
      RectangleCursors(ImageFrame&);
      ~RectangleCursors();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      double xlo() const;
      double ylo() const;
      double xhi() const;
      double yhi() const;
    public:   // ImageMarker interface
      void draw(QImage&);
    public:  // Cursors interface
      void mousePressEvent(double,double);
      void mouseReleaseEvent(double,double);
    private:
      void _set_edits ();
    public slots:
      void grab();
      void update_edits();
    signals:
      void changed();
    private:
      ImageFrame& _frame;
      double _x0, _y0;
      double _x1, _y1;
      QLineEdit* _edit_x0;
      QLineEdit* _edit_y0;
      QLineEdit* _edit_x1;
      QLineEdit* _edit_y1;
      unsigned   _xmax;
      unsigned   _ymax;
    };
  };
};

#endif
