#include "ami/app/EventFilter.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Xtc.hh"

using namespace Pds;

bool Ami::EventFilter::accept(Dgram* dg)
{
  if (_filters.empty())
    return true;

  iterate(&dg->xtc);

  for(std::list<Ami::UserFilter*>::iterator it=_filters.begin();
      it!=_filters.end(); it++)
    if (!(*it)-> accept())
      return false;

  return true;
}

int Ami::EventFilter::process(Xtc* xtc)
{
  if (xtc->extent < sizeof(Xtc) ||
      xtc->contains.id() >= TypeId::NumberOf)
    return 0;

  if (xtc->contains.id()==TypeId::Id_Xtc)
    iterate(xtc);
  else
    for(std::list<Ami::UserFilter*>::iterator it=_filters.begin();
        it!=_filters.end(); it++)
      (*it)-> event(xtc->src,
                    xtc->contains,
                    xtc->payload());
  return 1;
}
