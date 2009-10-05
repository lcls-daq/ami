#ifndef AmiQt_ImageColorControl_hh
#define AmiQt_ImageColorControl_hh

#include "ami/qt/QtPersistent.hh"

#include <QtGui/QGroupBox>

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QPixmap>

class QButtonGroup;
class QLabel;

namespace Ami {
  namespace Qt {
    class ImageColorControl : public QGroupBox, 
			      public QtPersistent {
      Q_OBJECT
    public:
      ImageColorControl(QWidget*, const QString& title);
      ~ImageColorControl();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      double scale() const;
      const QVector<QRgb>& color_table() const;
    public slots:
      void show_scale();
      void set_auto(bool);
      void zoom();
      void pan ();
      void set_palette(int);
    signals:
      void windowChanged();
    private:
      int     _scale;
      QString _title;
      QVector<QRgb>* _color_table;
      QLabel* _scale_max;
      QButtonGroup* _paletteGroup;
    };
  };
};

#endif
