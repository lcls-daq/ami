#ifndef AmiQt_ImageColorControl_hh
#define AmiQt_ImageColorControl_hh

#include "ami/data/QtPersistent.hh"

#include <QtGui/QGroupBox>

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QPixmap>
#include "ColorMaps.hh"

#include <string>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;

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
      enum Range { Fixed, Full, Dynamic, NRanges };
      Range  range   () const;  
      bool   linear  () const;
      float  pedestal() const;  // Fixed
      float  scale   () const;  // Fixed
      float  nsigma  () const;  // Auto2
      const QVector<QRgb>& color_table() const;
    public:
      void   full_range_setup   (double zmin, double zmax);
      void   dynamic_range_setup(double n, double sum, double sqsum);
    public:
      static std::string palette_set();
      static bool parse_palette_set(const char*);
      static const QVector<QRgb>& current_color_table();
    public slots:
      void show_scale();
      void reset(bool);
      void zoom();
      void pan ();
      void set_palette(int);
      void scale_changed();
      void range_changed(int);
    signals:
      void scaleChanged();
      void windowChanged();
    private:
      double         _scale   [NRanges];
      double         _pedestal[NRanges];
      QVector<QRgb>* _color_table;
      QLineEdit*     _scale_min;
      QLabel*        _scale_mid;
      QLineEdit*     _scale_max;
      QButtonGroup*  _paletteGroup;
      Range          _range;
      QCheckBox* _logscale_fixed;
      QCheckBox* _logscale_full;
      QLineEdit* _nsigma;
      QComboBox* _range_box;
    };
  };
};

#endif
