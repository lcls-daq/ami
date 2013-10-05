#include "LogicOr.hh"

using namespace Ami;

LogicOr::LogicOr(AbsFilter& a,AbsFilter& b) :
  CompoundFilter(AbsFilter::LogicOr,a,b)
{
}

LogicOr::~LogicOr()
{
}

bool  LogicOr::valid () const { return _a.valid () || _b.valid (); }
bool  LogicOr::accept() const { return _a.accept() || _b.accept(); }

AbsFilter* LogicOr::clone() const
{
  return new LogicOr(*_a.clone(),*_b.clone());
}
