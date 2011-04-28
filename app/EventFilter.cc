#include "ami/app/EventFilter.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <string>
using std::string;
using std::list;
using namespace Ami;

EventFilter::EventFilter(list<UserFilter*>& filters,
                         FeatureCache& cache) :
  _filters(filters),
  _cache  (cache),
  _enable (0)
{
  _index.resize(filters.size(),-1);
}

EventFilter::~EventFilter()
{
  for(list<UserFilter*>::iterator it=_filters.begin(); 
      it!=_filters.end(); it++)
    delete (*it);

}

void EventFilter::enable (unsigned o)
{
  _enable = o;
}

void EventFilter::configure   (const Xtc& xtc)
{
  for(list<UserFilter*>::iterator it=_filters.begin(); it!=_filters.end(); it++)
    (*it)->configure(xtc.src,
                     xtc.contains,
                     xtc.payload());
}

void Ami::EventFilter::add_to_cache()
{
  int i=0;
  for(list<Ami::UserFilter*>::iterator it=_filters.begin();
      it!=_filters.end(); it++) {
    string fname = string("UF:") + (*it)->name();
    _index[i++] = _cache.add(fname.c_str());
  }
}

bool Ami::EventFilter::accept(Dgram* dg)
{
  if (_filters.empty())
    return true;

  for(list<Ami::UserFilter*>::iterator it=_filters.begin();
      it!=_filters.end(); it++)
    (*it)-> clock(dg->seq.clock());

  iterate(&dg->xtc);

  bool result=true;
  int i=0;
  for(list<Ami::UserFilter*>::iterator it=_filters.begin();
      it!=_filters.end(); it++) {
    if ((*it)->accept())
      _cache.cache(_index[i++],1);
    else {
      result = result && ((_enable&(1<<i))==0);
      _cache.cache(_index[i++],0);
    }
  }
  return result;
}

int Ami::EventFilter::process(Xtc* xtc)
{
  if (xtc->extent < sizeof(Xtc) ||
      xtc->contains.id() >= TypeId::NumberOf)
    return 0;

  if (xtc->contains.id()==TypeId::Id_Xtc)
    iterate(xtc);
  else
    for(list<Ami::UserFilter*>::iterator it=_filters.begin();
        it!=_filters.end(); it++)
      (*it)-> event(xtc->src,
                    xtc->contains,
                    xtc->payload());
  return 1;
}
