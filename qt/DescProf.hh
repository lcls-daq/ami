#ifndef AmiQt_DescProf_hh
#define AmiQt_DescProf_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

class QComboBox;
class QRadioButton;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class DescProf : public QWidget {
      Q_OBJECT
    public:
      DescProf(const char*);
      DescProf(const char*,QComboBox*);
    public:
      void save(char*&) const;
      void load(const char*&);
    public slots:
      void set_variable(const QString&);
    public:
      QRadioButton* button();
      const QString& variable() const;
      unsigned bins() const;
      double   lo  () const;
      double   hi  () const;
    private:
      QRadioButton* _button;
      QLineEdit *_bins, *_lo, *_hi;
      QString _var;
    };
  };
};

#endif      
