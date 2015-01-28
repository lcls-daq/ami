#include "SingleShot.hh"

using namespace Ami;

SingleShot::SingleShot() :
  AbsFilter(AbsFilter::SingleShot),
  _fired(false)
{
}

SingleShot::~SingleShot()
{
}

bool SingleShot::valid () const
{
  return true;
}

bool SingleShot::accept() const
{
  bool result = !_fired;
  const_cast<SingleShot*>(this)->_fired=true;
  return result;
}

void*      SingleShot::_serialize(void* p) const { return p; }

AbsFilter* SingleShot::clone() const { return new SingleShot; }

std::string SingleShot::text() const { return std::string("ONCE"); }
