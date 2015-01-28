#ifndef AmiQt_DoubleEdit_hh
#define AmiQt_DoubleEdit_hh

#include <QtGui/QLineEdit>

namespace Ami {
  namespace Qt {
    class DoubleEdit : public QLineEdit {
      Q_OBJECT
    public:
      DoubleEdit(double v);
      ~DoubleEdit();
    public:
      double value() const { return _v; }
      void   value(double);
    public slots:
      void set_value();
    signals:
      void changed();
    private:
      double     _v;
    };
  };
};

#endif
