#ifndef AmiQt_ImageScale_hh
#define AmiQt_ImageScale_hh

#include <QtGui/QWidget>

class QString;
class QSpinBox;
class QLabel;
class QPixmap;

namespace Ami {
  namespace Qt {
    class ImageColorControl;
    class ImageScale : public QWidget {
      Q_OBJECT
    public:
      ImageScale(const QString& title,
		 const ImageColorControl&);
      ~ImageScale();
    public:
      unsigned value() const;
    public slots:
      void value_change(int);
      void scale_change();
    private:
      QSpinBox* _input;
      QLabel*   _canvas;
      const ImageColorControl& _color;
      QPixmap*  _pixmap;
    };
  };
};

#endif
