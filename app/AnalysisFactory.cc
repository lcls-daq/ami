#include "AnalysisFactory.hh"

#include "ami/data/Analysis.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/Message.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/server/ServerManager.hh"

using namespace Ami;


AnalysisFactory::AnalysisFactory(FeatureCache& cache,
				 ServerManager& srv) :
  _srv       (srv),
  _cds       ("Analysis"),
  _configured(Semaphore::EMPTY),
  _features  (cache)
{
}

AnalysisFactory::~AnalysisFactory()
{
}

FeatureCache& AnalysisFactory::features() { return _features; }

Cds& AnalysisFactory::discovery() { return _cds; }

//
//  The discovery cds has changed
//
void AnalysisFactory::discover () 
{
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++)
    delete *it;
  _analyses.clear();
  _srv.discover(); 
}

void AnalysisFactory::configure(unsigned       id,
				const Message& msg, 
				const char*    payload, 
				Cds&           cds)
{
  printf("configure\n");
  
  AnList newlist;
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    Analysis* a = *it;
    if (a->id() == id)
      delete a;
    else
      newlist.push_back(a);
  }
  _analyses = newlist;

  // create
  const char* const end = payload + msg.payload();
  while(payload < end) {
    const ConfigureRequest& req = *reinterpret_cast<const ConfigureRequest*>(payload);

    const Entry& input = *_cds.entry(req.input());      
    const char*  p     = reinterpret_cast<const char*>(&req+1);
    _analyses.push_back(new Analysis(id, input, req.output(),
				     cds, _features, p));

    payload += req.size();
  }

  _configured.give();
}

void AnalysisFactory::analyze  ()
{
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    (*it)->analyze();
  }
}

void AnalysisFactory::wait_for_configure() { _configured.take(); }
