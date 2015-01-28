#ifndef Ami_SelfExpression_hh
#define Ami_SelfExpression_hh

#include "ami/data/FeatureExpression.hh"

namespace Ami {

  class DescEntry;
  class Entry;

  class SelfExpression : public FeatureExpression {
  public:
    SelfExpression();
    ~SelfExpression();
  public:
    static const QString& self_sum();
    //    static const QString& self_max();
  public:
    Term* evaluate(FeatureCache&, const QString&, const Entry*&, const DescEntry&);
  };
};

#endif
