#include "ami/app/EventFilter.hh"
#include "ami/app/FilterExport.hh"

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

void EventFilter::enable (const ConfigureRequest& req,
			  const std::list<const Analysis*>& a,
			  const std::list<const EventHandler*>& e)
{
  const uint32_t* u = reinterpret_cast<const uint32_t*>(&req+1);
  const char* p = reinterpret_cast<const char*>(u+1);
  FilterFactory factory(_cache);
  AbsFilter* f = factory.deserialize(p);

  if ((*u) & 0x80000000) {
    FilterExport x(*f, e, a);
    x.write(p);
  }
  else {
    _enable = (*u) & 0x7fffffff;
    delete _f;
    _f = f ? f : new RawFilter;
  }
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
    for(list<Ami::UserModule*>::iterator it=_filters.begin();
        it!=_filters.end(); it++)
      switch(xtc->src.level()) {
      case Level::Source:
        (*it)-> event(static_cast<const Pds::DetInfo&>(xtc->src),
                      xtc->contains,
                      xtc->damage,
                      xtc->payload());
        break;
      case Level::Reporter:
        (*it)-> event(static_cast<const Pds::BldInfo&>(xtc->src),
                      xtc->contains,
                      xtc->damage,
                      xtc->payload());
        break;
      default:
        (*it)-> event(static_cast<const Pds::ProcInfo&>(xtc->src),
                      xtc->contains,
                      xtc->damage,
                      xtc->payload());
        break;
      }
  }
  else if (_seq->service()==TransitionId::Configure) {
    for(list<Ami::UserModule*>::iterator it=_filters.begin();
        it!=_filters.end(); it++)
      if (xtc->damage.value()==0)
        switch(xtc->src.level()) {
        case Level::Source:
          (*it)-> configure(static_cast<const Pds::DetInfo&>(xtc->src),
                            xtc->contains,
                            xtc->payload());
          break;
        case Level::Reporter:
          (*it)-> configure(static_cast<const Pds::BldInfo&>(xtc->src),
                            xtc->contains,
                            xtc->payload());
          break;
        default:
          (*it)-> configure(static_cast<const Pds::ProcInfo&>(xtc->src),
                            xtc->contains,
                            xtc->payload());
          break;
        }
  }
  else
    ;
  return 1;
}
