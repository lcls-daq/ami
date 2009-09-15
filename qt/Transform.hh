#ifndef AmiQt_Transform_hh
#define AmiQt_Transform_hh

#include "ami/data/AbsTransform.hh"
#include <QtGui/QWidget>

#include <list>

class QLineEdit;
class QVBoxLayout;
class QLabel;
class QPushButton;
class QString;

namespace Ami {

  class Term;
  
  namespace Qt {

    class TransformConstant;
    class ExprValidator;
    class QtBase;

    class Transform : public QWidget,
		      public Ami::AbsTransform {
      Q_OBJECT
    public:
      Transform(const QString& title,
		const QString& axis);
      ~Transform();
    public:
      double operator()(double) const;
    public:
//       void read (istream&) {}
//       void write(ostream&) const {}
    public slots:
      void add   ();
      void remove(const QString&);
      void calc  ();
      void apply ();
      void clear ();
      void save  ();
      void load  ();
    signals:
      void changed();
    private:
      QString    _name;
      QLineEdit* _expr;
      QLineEdit* _new_name;
      QLineEdit* _new_value;
      QVBoxLayout* _clayout;
      std::list<TransformConstant*> _constants;

      Ami::Term* _term;
      mutable double _axis;
    };
  };
};

#endif
