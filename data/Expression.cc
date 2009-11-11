#include "Expression.hh"

#include <QtCore/QRegExp>
#include <QtCore/QChar>

#include <math.h>

//#define DBUG

#define Binary(classname,o) \
    class classname : public Term { \
    public: \
      classname(Term* a,Term* b) : _a(*a), _b(*b) {} \
      ~classname() { delete &_a; delete &_b; }	   \
    public: \
      double evaluate() const { return o; } \
    private: \
      Term& _a; \
      Term& _b; \
    }

namespace Ami {
  Binary(Add,_a.evaluate()+_b.evaluate());
  Binary(Sub,_a.evaluate()-_b.evaluate());
  Binary(Mul,_a.evaluate()*_b.evaluate());
  Binary(Div,_a.evaluate()/_b.evaluate());
  Binary(Exp,pow(_a.evaluate(),_b.evaluate()));
};

using namespace Ami;

// must be ascii characters (due to serialization)
static QChar _exponentiate('^');
static QChar _multiply    ('*');
static QChar _divide      ('/');
static QChar _add         ('+');
static QChar _subtract    ('-');

const QChar& Expression::exponentiate() { return _exponentiate; }
const QChar& Expression::multiply    () { return _multiply; }
const QChar& Expression::divide      () { return _divide; }
const QChar& Expression::add         () { return _add; }
const QChar& Expression::subtract    () { return _subtract; }

Expression::Expression(const std::list<Variable*>& variables) :
  _variables(variables)
{
}

Expression::~Expression() {}

QString& Expression::_process(QString& text,const QChar& o) 
{
  QRegExp constantMatch   ("[\\+\\-]?[0-9]*[\\.]?[0-9]*");
  QRegExp revConstantMatch("[0-9]*[\\.]?[0-9]*[\\+\\-]?");
  QRegExp termMatch       ("\\[[0-9a-fA-F]+\\]");

  int index, first = 0, last = 0;
  while( (index=text.indexOf(o))!=-1 ) {  // left-to-right
    //  while( (index=text.lastIndexOf(o))!=-1 ) {   // right-to-left
#ifdef DBUG
    printf("_process(%s) %s\n",qPrintable(QString(o)),qPrintable(text));
#endif
    //  
    //  Note that QRegExp::lastIndexIn will only match one character of my constant regexp,
    //    so I reverse the target text and use indexIn instead.
    //
    QString revText;
    for(int i=0; i<index; i++)
      revText.prepend(text[i]);
      
    Term* b = NULL;
    if (termMatch.indexIn(text,index+1)==index+1) {
      int len = termMatch.matchedLength();
      last = index+len;
      QString str = text.mid(index+2,len-2);
      unsigned ub = str.toULong(0,16);
      b = reinterpret_cast<Term*>(ub);
#ifdef DBUG
      printf("Found term %s : %p %x\n",qPrintable(str),b,ub);
#endif
    }
    else if (text[index+1].isLetter()) {  // variable
      // loop through variable names and test against this part of the string
      last=-1;
      QString str = text.mid(index+1);
      for(VarList::const_iterator it = _variables.begin(); it != _variables.end(); it++) {
	if (str.startsWith((*it)->name())) {
	  b = (*it)->clone();
	  last = index+(*it)->name().size();
#ifdef DBUG
	  printf("Found variable %s at %s [%p]\n",qPrintable((*it)->name()),qPrintable(str),b);
#endif
	  break;
	}
      }
      if (last<0) {
	printf("Failed to find variable at %s\n",qPrintable(str));
	text.clear();
	return text;
      }
    }
    else if (constantMatch.indexIn(text, index+1)==index+1) { // constant
      int len = constantMatch.matchedLength();
      QString str = text.mid(index+1,len);
      double v = str.toDouble();
      b = new Constant("constant",v);
      last = index+len;
#ifdef DBUG
      printf("Found constant %s : %g\n",qPrintable(str),v);
#endif
    }
    else {
      printf("Unrecognized input at %s\n",qPrintable(text.mid(index+1)));
    }
    Term* a = NULL;
    if ((termMatch.lastIndexIn(text,index-1)+termMatch.matchedLength())==index) {
      int len = termMatch.matchedLength();
      first = index-len;
      QString str = text.mid(first+1,len-2);
      unsigned ua = str.toULong(0,16);
      a = reinterpret_cast<Term*>(ua);
#ifdef DBUG
      printf("Found term %s : %p %x\n",qPrintable(str),a,ua);
#endif
    }
    else if (text[index-1].isLetter()) {  // variable
      // loop through variable names and test against this part of the string
      first=-1;
      for(VarList::const_iterator it = _variables.begin(); it != _variables.end(); it++) {
	QString str = text.mid(index-(*it)->name().size());
	if (str.startsWith((*it)->name())) {
	  a = (*it)->clone();
	  first = index-(*it)->name().size();
#ifdef DBUG
	  printf("Found variable %s at %s [%p]\n",qPrintable((*it)->name()),qPrintable(str),a);
#endif
	  break;
	}
      }
      if (first<0) {
	printf("Failed to find variable at %s\n",qPrintable(text.mid(0,index)));
	text.clear();
	return text;
      }
    }
//     else if ((constantMatch.lastIndexIn(text, index-1)+constantMatch.matchedLength())==index) { // constant
//       int len = constantMatch.matchedLength();
    else if (revConstantMatch.indexIn(revText,0)==0) { // constant
      int len = revConstantMatch.matchedLength();
      first = index-len;
      QString str = text.mid(first,len);
      double v = str.toDouble();
      a = new Constant("constant",v);
#ifdef DBUG
      printf("Found constant %s : %g\n",qPrintable(str),v);
#endif
    }
    else {
      printf("Unrecognized input at %s\n",qPrintable(text.mid(0,index-1)));
    }
    text.remove(first,last-first+1);
    Term* B;
    if      (o==_exponentiate) { B = new Exp(a,b); }
    else if (o==_multiply    ) { B = new Mul(a,b); }
    else if (o==_divide      ) { B = new Div(a,b); }
    else if (o==_add         ) { B = new Add(a,b); }
    else if (o==_subtract    ) { B = new Sub(a,b); }
    else { B = 0; } // can't get here

    text.insert(first,QString("[%1]").arg((unsigned long)B,0,16));
  }
  return text;
}

QString& Expression::_process(QString& text)
{
  // 
  //  Special case is the monomial (no operators)
  //
  for(VarList::const_iterator it = _variables.begin(); it != _variables.end(); it++) {
    if (text.compare((*it)->name())==0) {
      text = QString("[%1]").arg((unsigned long)((*it)->clone()),0,16);
      return text;
    }
  }
  bool ok;
  double v=text.toDouble(&ok);
  if (ok) {
    text = QString("[%1]").arg((unsigned long)(new Constant("constant",v)),0,16);
    return text;
  }
  
  _process(text,_exponentiate);
  _process(text,_divide);
  _process(text,_multiply);
  _process(text,_subtract);
  _process(text,_add);
  return text;
}
	 
Term* Expression::evaluate(const QString& e) 
{
  QString text(e);
#ifdef DBUG
  printf("_evaluate %s\n",qPrintable(text));
#endif

  //  process "()" first
  int end;
  while( (end=text.indexOf(')'))!=-1 ) {
    int start=text.lastIndexOf('(',end);
    QString t = text.mid(start+1,end-start-1);
    text.replace(start,end-start+1,_process(t));
  }

  _process(text);

  bool ok;
  Term* t = reinterpret_cast<Term*>(text.mid(1,text.size()-2).toULong(&ok,16));
#ifdef DBUG
  printf("Result is (%s) %p %s\n",ok ? "OK" : "Not OK", t,qPrintable(text));
#endif
  return ok ? t : 0;
}

