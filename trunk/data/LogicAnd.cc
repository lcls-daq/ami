#include "LogicAnd.hh"

#include <sstream>

using namespace Ami;

LogicAnd::LogicAnd(AbsFilter& a,AbsFilter& b) :
  CompoundFilter(AbsFilter::LogicAnd,a,b)
{
}

LogicAnd::~LogicAnd()
{
}

bool  LogicAnd::valid () const { return _a.valid () && _b.valid (); }
bool  LogicAnd::accept() const { return _a.accept() && _b.accept(); }

AbsFilter* LogicAnd::clone() const 
{
  return new LogicAnd(*_a.clone(),*_b.clone());
}

std::string LogicAnd::text() const
{
  std::ostringstream s;
  s << "(" << _a.text() << ") AND (" << _b.text() << ")";
  return s.str();
}
