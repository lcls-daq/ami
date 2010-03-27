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
  _sem       (Semaphore::FULL),
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
  _sem.take();
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    Analysis* an = *it;
    delete an;
  }
  _analyses.clear();
  _sem.give();
  _srv.discover(); 
}

void AnalysisFactory::configure(unsigned       id,
				const Message& msg, 
				const char*    payload, 
				Cds&           cds)
{
  printf("configure\n");

  _sem.take();
  AnList newlist;
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    Analysis* a = *it;
    if (a->id() == id) {
      delete a;
    }
    else
      newlist.push_back(a);
  }
  _analyses = newlist;

  // create
  const char* const end = payload + msg.payload();
  while(payload < end) {
    const ConfigureRequest& req = *reinterpret_cast<const ConfigureRequest*>(payload);

    const Cds& input_cds = req.source() == ConfigureRequest::Discovery ? _cds : cds;
    const Entry& input = *input_cds.entry(req.input());
    const char*  p     = reinterpret_cast<const char*>(&req+1);
    Analysis* a = new Analysis(id, input, req.output(),
			       cds, _features, p);
    _analyses.push_back(a);
    payload += req.size();
  }
  _sem.give();

  _configured.give();
}

void AnalysisFactory::analyze  ()
{
  _sem.take();
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    (*it)->analyze();
  }
  _sem.give();
}

void AnalysisFactory::wait_for_configure() { _configured.take(); }
