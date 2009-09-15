#ifndef AmiQt_DescTH1F_hh
#define AmiQt_DescTH1F_hh

#include <QtGui/QWidget>

class QRadioButton;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class DescTH1F : public QWidget {
      Q_OBJECT
    public:
      DescTH1F(const char* name);
    public:
      QRadioButton* button();
      unsigned bins() const;
      double   lo  () const;
      double   hi  () const;
    private:
      QRadioButton* _button;
      QLineEdit *_bins, *_lo, *_hi;
    };
  };
};

#endif      
