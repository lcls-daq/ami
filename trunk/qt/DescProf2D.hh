#ifndef AmiQt_DescProf2D_hh
#define AmiQt_DescProf2D_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

class QRadioButton;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class FeatureRegistry;
    class DescProf2D : public QWidget {
      Q_OBJECT
    public:
      DescProf2D(const char*, FeatureRegistry*);
    public:
      void save(char*&) const;
      void load(const char*&);
    public slots:
      void xcalc();
      void ycalc();
    public:
      QRadioButton* button();
      unsigned xbins() const;
      double   xlo  () const;
      double   xhi  () const;
      QString  xexpr() const;
      unsigned ybins() const;
      double   ylo  () const;
      double   yhi  () const;
      QString  yexpr() const;
    private:
      QRadioButton* _button;
      QLineEdit *_xbins, *_xlo, *_xhi;
      QLineEdit* _xexpr;
      QLineEdit *_ybins, *_ylo, *_yhi;
      QLineEdit* _yexpr;
      FeatureRegistry* _registry;
    };
  };
};

#endif      
