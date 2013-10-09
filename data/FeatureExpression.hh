#ifndef Ami_FeatureExpression_hh
#define Ami_FeatureExpression_hh

#include "ami/data/Expression.hh"

#include <list>

namespace Ami {

  class FeatureCache;
  class Feature : public Term {
  public:
    Feature(FeatureCache&, unsigned);
    ~Feature();
  public:
    double evaluate() const;
    bool   valid   () const;
  private:
    FeatureCache& _features;
    unsigned      _index;
    mutable bool  _damaged;
  };

  class FeatureExpression : public Expression {
  public:
    FeatureExpression();
    ~FeatureExpression();
  public:
    Term* evaluate(FeatureCache&, const QString&);
  private:
    std::list<Variable*> _variables;
  };
};

#endif
