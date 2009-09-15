#ifndef AmiQt_DescChart_hh
#define AmiQt_DescChart_hh

#include <QtGui/QWidget>

class QRadioButton;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class DescChart : public QWidget {
      Q_OBJECT
    public:
      DescChart(const char* name, double dpt);
    public:
      QRadioButton* button();
      unsigned pts() const;
      double   dpt() const;
    private:
      QRadioButton* _button;
      QLineEdit *_pts;
      double     _dpt;
    };
  };
};

#endif      
