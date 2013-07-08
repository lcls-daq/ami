#include "ami/app/EventFilter.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/FilterFactory.hh"
#include "ami/data/RawFilter.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <string>
using std::string;
using std::list;
using namespace Ami;

EventFilter::EventFilter(list<UserModule*>& filters,
                         FeatureCache& cache) :
  _filters(filters),
  _cache  (cache),
  _f      (new RawFilter),
  _enable (0)
{
}

EventFilter::~EventFilter()
{
  for(list<UserModule*>::iterator it=_filters.begin(); 
      it!=_filters.end(); it++)
    delete (*it);
  delete _f;
}

void EventFilter::enable (const ConfigureRequest& req)
{
  const uint32_t* u = reinterpret_cast<const uint32_t*>(&req+1);
  _enable = *u;

  delete _f;

  const char* p = reinterpret_cast<const char*>(u+1);
  FilterFactory factory(_cache);
  AbsFilter* f = factory.deserialize(p);
  _f = f ? f : new RawFilter;
}

void EventFilter::reset  ()
{
  for(list<UserModule*>::iterator it=_filters.begin(); it!=_filters.end(); it++)
    (*it)->reset(_cache);
}

void EventFilter::configure   (Dgram* dg)
{
  for(list<Ami::UserModule*>::iterator it=_filters.begin();
      it!=_filters.end(); it++)
    (*it)-> clock(dg->seq.clock());

  _seq = &dg->seq;
  iterate(&dg->xtc);
}

bool Ami::EventFilter::accept(Dgram* dg)
{
  if (_filters.empty())
    return true;

  for(list<Ami::UserModule*>::iterator it=_filters.begin();
      it!=_filters.end(); it++)
    (*it)-> clock(dg->seq.clock());

  _seq = &dg->seq;
  iterate(&dg->xtc);

  bool result=true;
  int i=0;
  for(list<Ami::UserModule*>::iterator it=_filters.begin();
      it!=_filters.end(); it++) {
    if (!(*it)->accept())
      result = result && ((_enable&(1<<i))==0);
  }
  return result;
}

bool Ami::EventFilter::accept()
{
  return _f->accept();
}

int Ami::EventFilter::process(Xtc* xtc)
{
  if (xtc->extent < sizeof(Xtc) ||
      xtc->contains.id() >= TypeId::NumberOf)
    return 0;

  if (xtc->contains.id()==TypeId::Id_Xtc)
    iterate(xtc);
  else if (_seq->service()==TransitionId::L1Accept) {
    if (xtc->damage.value()==0)
      for(list<Ami::UserModule*>::iterator it=_filters.begin();
          it!=_filters.end(); it++)
        (*it)-> event(xtc->src,
                      xtc->contains,
                      xtc->payload());
  }
  else if (_seq->service()==TransitionId::Configure) {
    for(list<Ami::UserModule*>::iterator it=_filters.begin();
        it!=_filters.end(); it++)
      if (xtc->damage.value()==0)
        (*it)-> configure(xtc->src,
                          xtc->contains,
                          xtc->payload());
  }
  else
    ;
  return 1;
}
