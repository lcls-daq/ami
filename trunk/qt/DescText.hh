#ifndef AmiQt_DescText_hh
#define AmiQt_DescText_hh

#include "ami/data/DescScalar.hh"

#include <QtGui/QWidget>

class QComboBox;
class QRadioButton;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class DescText : public QWidget {
      Q_OBJECT
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      DescText(const char* name);
    public:
      QRadioButton* button();
      Ami::DescScalar::Stat stat() const;
      unsigned pts() const;
      unsigned dpt() const;
    private:
      QRadioButton* _button;
      QComboBox*    _stat;
      QLineEdit *_pts;
      QLineEdit* _dpt;
    };
  };
};

#endif      
