#include "PVState.hh"

#include "Ca.hh"

using namespace Ami;


bool CaNameCompare::operator()(const Ca* a, const Ca* b) const
{
  return strcmp(a->name(),b->name());
}


PVState::PVState() {}

PVState::~PVState() {}

double PVState::value(const Ca* src) const
  throw(Event)
{
  MapType::const_iterator it = _map.find(src);
  if (it == _map.end()) throw Event(src->name());
  return it->second;
}

void PVState::add(const Ca& ca)
{
  _map.insert(ElType(&ca, ca.value()));
}

