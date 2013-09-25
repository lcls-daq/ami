#ifndef Ami_LogicAnd_hh
#define Ami_LogicAnd_hh

#include "CompoundFilter.hh"

namespace Ami {
  class LogicAnd : public CompoundFilter {
  public:
    LogicAnd(AbsFilter&,AbsFilter&);
    ~LogicAnd();
  public:
    bool  accept() const;
    AbsFilter* clone() const;
  };
};

#endif
