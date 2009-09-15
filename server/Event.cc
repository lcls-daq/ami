#include "Event.hh"

using namespace Ami;

bool DetInfoCompare::operator()(const Pds::DetInfo& a, const Pds::DetInfo& b) const
{
    return (a.log() < b.log()) || (a.log()==b.log() && a.phy()<b.phy());
}



Event::Event() {}

Event::~Event() {}

const Pds::Xtc& Event::value(const Pds::DetInfo& src) const
  throw(NoValue)
{
  MapType::const_iterator it = _map.find(src);
  if (it == _map.end()) throw NoValue(Pds::DetInfo::name(src));
  return *(it->second);
}

void Event::add(const Pds::Xtc& xtc)
{
  _map.insert(ElType(reinterpret_cast<const Pds::DetInfo&>(xtc.src),&xtc));
}

