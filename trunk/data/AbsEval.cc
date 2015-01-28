#include "ami/data/AbsEval.hh"

#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryProf2D.hh"

using namespace Ami;

class MeanEval : public AbsEval {
public:
  MeanEval() {}
  ~MeanEval() {}
public:
  double evaluate(const EntryScalar& e, const EntryScalar& b, double n) const 
  { return (e.sum()-b.sum())/n; }
  double evaluate(const EntryScan&   e, unsigned b) const
  { return e.ymean(b); }
  double evaluate(const EntryProf&   e, unsigned b) const
  { return e.ymean(b); }
  double evaluate(const EntryProf2D& e, unsigned x, unsigned y) const
  { return e.zmean(x,y); }
};

class StdDevEval : public AbsEval {
public:
  StdDevEval() {}
  ~StdDevEval() {}
public:
  double evaluate(const EntryScalar& e, const EntryScalar& b, double n) const 
  { double m = (e.sum()-b.sum())/n;
    double v = (e.sqsum()-b.sqsum() - m*m/n)/n;
    return (v>0) ? sqrt(v) : 0; }
  double evaluate(const EntryScan&   e, unsigned b) const
  { return e.sigma(b); }
  double evaluate(const EntryProf&   e, unsigned b) const
  { return e.sigma(b); }
  double evaluate(const EntryProf2D& e, unsigned x, unsigned y) const
  { return e.sigma(x,y); }
};

class InterceptEval : public AbsEval {
public:
  InterceptEval() {}
  ~InterceptEval() {}
public:
  double evaluate(const EntryScalar& e, const EntryScalar& b, double n) const 
  { return (e.sqsum()-b.sqsum())/n; }
  double evaluate(const EntryScan&   e, unsigned b) const
  { return e.intercept(b); }
  double evaluate(const EntryProf&   e, unsigned b) const
  { return e.intercept(b); }
  double evaluate(const EntryProf2D& e, unsigned x, unsigned y) const
  { return e.intercept(x,y); }
};

AbsEval* AbsEval::lookup(DescEntry::Stat stat)
{
  switch(stat) {
  case DescEntry::StdDev   :    return new StdDevEval;
  case DescEntry::Intercept:    return new InterceptEval;
  default:                      return new MeanEval;
  }
}
