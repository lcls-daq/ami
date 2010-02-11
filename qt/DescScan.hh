#ifndef AmiQt_DescScan_hh
#define AmiQt_DescScan_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

class QRadioButton;
class QLineEdit;

namespace Ami {
  namespace Qt {
    class FeatureBox;
    class DescScan : public QWidget {
      Q_OBJECT
    public:
      DescScan(const char*);
    public:
      void save(char*&) const;
      void load(const char*&);
    public:
      QRadioButton* button();
      QString  expr() const;
      QString  feature() const;
      unsigned bins() const;
    private:
      QRadioButton* _button;
      QLineEdit*  _bins;
      FeatureBox* _features;
    };
  };
};

#endif      