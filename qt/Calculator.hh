#ifndef AmiQt_Calculator_hh
#define AmiQt_Calculator_hh

#include <QtGui/QDialog>
#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QColor;

namespace Ami {
  namespace Qt {
    class CalculatorButton;

    class Calculator : public QDialog {
      Q_OBJECT
    public:
      Calculator(const QString&     title,
		 const QString&     reset,
		 const QStringList& variables,    // pre-defined variables {cursor bin, filter condition, ..}
		 const QStringList& var_var_ops,  // binary operations only between variables
		 const QStringList& var_con_ops); // binary operations between any combination of variables,constants
    public:
      QString result() const;
    private slots:
      void digitClicked();
      void pointClicked();
      void parenthesesClicked();
      //      void changeSignClicked();
      void backspaceClicked();
      void clear();
      void clearAll();
      void variableClicked();
      void varvarClicked();
      void varconClicked();

    private:
      CalculatorButton *createButton(const QString &text, const QColor &color,
				     const char *member);

    private:
      QString     _reset;
      QStringList _variables;
      QStringList _varvarops;
      QStringList _varconops;
      QLineEdit*  _display;
    };
  };
};
#endif
