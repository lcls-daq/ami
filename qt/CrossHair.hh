#ifndef AmiQt_CrossHair_hh
#define AmiQt_CrossHair_hh

#include "ami/qt/Cursors.hh"
#include "ami/qt/ImageMarker.hh"

#include <QtCore/QObject>

class QGridLayout;

namespace Ami {
  namespace Qt {

    class ImageGridScale;
    class ImageFrame;
    class CrossHairLocation;

    class CrossHair : public QObject,
		      public Cursors, 
		      public ImageMarker {
      Q_OBJECT
    public:
      CrossHair(ImageGridScale& parent, QGridLayout& layout, unsigned row, bool grab);
      ~CrossHair();
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      double column() const;
      double row   () const;
      void set_scale(double,double);
      void disable_grab();

      void mousePressEvent  (double x, double y);
      void mouseMoveEvent   (double, double) {}
      void mouseReleaseEvent(double, double) {}
      void draw(QImage& img);

      static void layoutHeader(QGridLayout& layout);
    signals:
      void changed();
      void value_updated(QString);
    public slots:
      void grab_cursor();
      void setVisible(bool);
      void update_value(QString);
    private:
      ImageGridScale& _parent;
      ImageFrame&   _frame;
      CrossHairLocation* _column_edit;
      CrossHairLocation* _row_edit;
      CrossHairLocation* _value;
      bool _visible;
      double _scalex,_scaley;
    };
  };
};

#endif
