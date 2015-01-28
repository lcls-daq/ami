#ifndef Ami_AbsEval_hh
#define Ami_AbsEval_hh

#include "ami/data/DescEntry.hh"

namespace Ami {
  class EntryScalar;
  class EntryScan;
  class EntryProf;
  class EntryProf2D;
  class AbsEval {
  public:
    virtual ~AbsEval() {}
    virtual double evaluate(const EntryScalar&,const EntryScalar&,double) const = 0;
    virtual double evaluate(const EntryScan&  , unsigned) const = 0;
    virtual double evaluate(const EntryProf&  , unsigned) const = 0;
    virtual double evaluate(const EntryProf2D&, unsigned, unsigned) const = 0;
  public:
    static AbsEval* lookup(DescEntry::Stat);
  };
};

#endif
