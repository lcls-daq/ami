#ifndef Ami_Expression_hh
#define Ami_Expression_hh

#include "ami/service/Exception.hh"

#include <QtCore/QString>

#include <list>

class QChar;

namespace Ami {

  /**
   *   The base class for evaluating an algebraic expression
   */
  class Term {
  public:
    virtual ~Term() {}
  public:
    ///  Return the result of evaluating this term of the expression
    virtual double evaluate() const = 0;
  };

  /**
   *   A term representing a named event variable in an algebraic expression
   */
  class Variable : public Term {
  public:
    virtual ~Variable() {}
  public:
    ///  Clone the variable and its reference to the event data
    virtual Variable*      clone() const = 0;
    ///  Return the variable name as used in the expression
    virtual const QString& name () const = 0;
  };

  /**
   *   A term representing a named constant value in an algebraic expression
   */
  class Constant : public Variable {
  public:
    ///  Constructor from the name and value of the constant
    Constant(const QString& name, double v) : _name(name), _v(v) {}
    ~Constant() {}
  public:
    ///  Clone the named constant value
    Variable*      clone   () const { return new Constant(_name,_v); }
    ///  Return the name of the constant
    const QString& name    () const { return _name; }
    ///  Return the value of the constant
    double         evaluate() const { return _v; }
  private:
    QString _name;
    double  _v;
  };

  /**
   *   A class for evaluating an algebraic expression in terms
   *   of operators and names of constants and event variables.
   */
  class Expression {
  public:
    ///  Initialize the parser from the list of variable and constant names
    Expression(const std::list<Variable*>&);
    ~Expression();
  public:
    ///  Parse the expression and return an object that can be evaluated each event
    Term* evaluate(const QString&);
  public:
    ///  Character used to express the exponentiation operator
    static const QChar& exponentiate();
    ///  Character used to express the multiplication operator
    static const QChar& multiply    ();
    ///  Character used to express the division operator
    static const QChar& divide      ();
    ///  Character used to express the addition operator
    static const QChar& add         ();
    ///  Character used to express the subtraction operator
    static const QChar& subtract    ();
    ///  Character string for a (unnamed) constant value
    static QString      constant(double);
  private:
    QString& _process(QString&);
    QString& _process(QString&,const QChar&) throw(Event);
  private:
    typedef std::list<Variable*> VarList;
    const VarList& _variables;
  };
};

#endif
		 
