#ifndef Ami_LogicOr_hh
#define Ami_LogicOr_hh

#include "CompoundFilter.hh"

namespace Ami {
  class LogicOr : public CompoundFilter {
  public:
    LogicOr(AbsFilter&,AbsFilter&);
    ~LogicOr();
  public:
    bool  valid () const;
    bool  accept() const;
    AbsFilter* clone() const;
    std::string text() const;
  };
};

#endif
