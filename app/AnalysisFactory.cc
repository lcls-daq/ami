#include "AnalysisFactory.hh"

#include "ami/app/SummaryAnalysis.hh"
#include "ami/data/Analysis.hh"
#include "ami/data/UserAnalysis.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/Message.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/server/ServerManager.hh"

using namespace Ami;


AnalysisFactory::AnalysisFactory(FeatureCache&  cache,
				 ServerManager& srv,
				 UserAnalysis*  user) :
  _srv       (srv),
  _cds       ("Analysis"),
  _configured(Semaphore::EMPTY),
  _sem       (Semaphore::FULL),
  _features  (cache),
  _user      (user)
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
  printf("AnalysisFactory::discover complete\n");
}

void AnalysisFactory::configure(unsigned       id,
				const Message& msg, 
				const char*    payload, 
				Cds&           cds)
{
  printf("AnalysisFactory::configure\n");

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

    if (req.source() == ConfigureRequest::Summary) {
      SummaryAnalysis::instance().clear();
      SummaryAnalysis::instance().create(cds);
    }
    else if (req.source() == ConfigureRequest::User) {
      if (_user) {
	_user->clear();
	_user->create(cds);
      }
    }
    else {
      const Cds& input_cds = req.source() == ConfigureRequest::Discovery ? _cds : cds;
      const Entry* input = input_cds.entry(req.input());
      if (!input) {
	printf("AnalysisFactory::configure failed input for configure request:\n");
	printf("\tinp %d  out %d  size %d\n",req.input(),req.output(),req.size());
      }
      else if (req.state()==ConfigureRequest::Create) {
	const char*  p     = reinterpret_cast<const char*>(&req+1);
	Analysis* a = new Analysis(id, *input, req.output(),
				   cds, _features, p);
	_analyses.push_back(a);
      }
      else if (req.state()==ConfigureRequest::SetOpt) {
        unsigned o = *reinterpret_cast<const unsigned*>(&req+1);
        printf("Set options %x on entry %p\n",o,input);
        const_cast<Ami::DescEntry&>(input->desc()).options(o);
      }
    }
    payload += req.size();
  }
  _sem.give();

  _configured.give();
}

void AnalysisFactory::analyze  ()
{
  _sem.take();
  SummaryAnalysis::instance().analyze();
  if (_user) _user->analyze();
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    (*it)->analyze();
  }
  _sem.give();
}

void AnalysisFactory::wait_for_configure() { _configured.take(); }
