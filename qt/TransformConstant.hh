#ifndef AmiQt_TransformConstant_hh
#define AmiQt_TransformConstant_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

namespace Ami {
  namespace Qt {
    class TransformConstant : public QWidget {
      Q_OBJECT
    public:
      TransformConstant(const QString& name, double value);
      ~TransformConstant();
    public:
      const QString& name () const;
      double         value() const;
    public slots:
      void remove();
    signals:
      void removed(const QString&);
    private:
      QString _name;
      double  _value;
    };
  };
};

#endif
