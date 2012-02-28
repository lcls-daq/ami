#include "AnalysisFactory.hh"

#include "ami/app/SummaryAnalysis.hh"
#include "ami/app/EventFilter.hh"
#include "ami/data/Analysis.hh"
#include "ami/data/UserModule.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/Message.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/server/ServerManager.hh"

using namespace Ami;

AnalysisFactory::AnalysisFactory(std::vector<FeatureCache*>&  cache,
				 ServerManager& srv,
                                 UList&         user,
                                 EventFilter&   filter) :
  _srv       (srv),
  _cds       ("Analysis"),
  _ocds      ("Hidden"),
  _features  (cache),
  _user      (user),
  _filter    (filter),
  _configured(false)
{
  pthread_mutex_init(&_mutex, NULL);
  pthread_cond_init(&_condition, NULL);
  EntryFactory::source(*_features[PostAnalysis]);
}

AnalysisFactory::~AnalysisFactory()
{
}

std::vector<FeatureCache*>& AnalysisFactory::features() { return _features; }

Cds& AnalysisFactory::discovery() { return _cds; }
Cds& AnalysisFactory::hidden   () { return _ocds; }

//
//  The discovery cds has changed
//
void AnalysisFactory::discover(bool waitForConfigure) 
{
  pthread_mutex_lock(&_mutex);
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    Analysis* an = *it;
    delete an;
  }
  _analyses.clear();
  _configured = false;
  _srv.discover(); 
  if (waitForConfigure) {
    // waitingThread_ = pthread_self();
    for (;;) {
      if (_configured) {
        break;
      }
      pthread_cond_wait(&_condition, &_mutex);
    }
    pthread_mutex_unlock(&_mutex);
  }
}

void AnalysisFactory::configure(unsigned       id,
				const Message& msg, 
				const char*    payload, 
				Cds&           cds)
{
  pthread_mutex_lock(&_mutex);
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
    
    if (req.size()==0) {
      const uint32_t* p = reinterpret_cast<const uint32_t*>(&req);
      printf("AnalysisFactory::configure received corrupt request from id %d\n [%08x %08x %08x %08x %08x %08x]\n",
             id, p[0], p[1], p[2], p[3], p[4], p[5] );
      break;
    }

    if (req.source() == ConfigureRequest::Summary) {
      SummaryAnalysis::instance().clear();
      SummaryAnalysis::instance().create(cds);
    }
    else if (req.source() == ConfigureRequest::User) {
      for(UList::iterator it=_user.begin(); it!=_user.end(); it++) {
        (*it)->clear();
        (*it)->create(cds);
      }
    }
    else if (req.source() == ConfigureRequest::Filter) {
      unsigned o = *reinterpret_cast<const uint32_t*>(&req+1);
      _filter.enable(o);
    }
    else {
      const Cds* pcds = 0;
      char *reqType = NULL;
      switch(req.source()) {
        case ConfigureRequest::Discovery: pcds = &_cds; reqType = "Discovery"; break;
        case ConfigureRequest::Analysis : pcds = & cds; reqType = "Analysis"; break;
        case ConfigureRequest::Hidden   : pcds = &_ocds; reqType = "Hidden"; break;
        default:
          printf("AnalysisFactory::configure unknown source ConfigureRequest::%d\n",req.source());
          break;
      }
      if (!pcds) {
        printf("AnalysisFactory::configure failed (null pcds) for ConfigureRequest::%s\n", reqType);
        printf("\tinp %d  out %d  size %d\n",req.input(),req.output(),req.size());
        break;
      }
      const Cds& input_cds = *pcds;
      const Entry* input = input_cds.entry(req.input());
      if (!input) {
        printf("AnalysisFactory::configure failed (null input) for ConfigureRequest::%s\n", reqType);
        printf("\tinp %d  out %d  size %d\n",req.input(),req.output(),req.size());
        input_cds.showentries();
      }
      else if (req.state()==ConfigureRequest::Create) {
        const char*  p     = reinterpret_cast<const char*>(&req+1);
        Analysis* a = new Analysis(id, *input, req.output(),
                                   cds, *_features[req.scalars()], p);
        _analyses.push_back(a);
      }
      else if (req.state()==ConfigureRequest::SetOpt) {
        unsigned o = *reinterpret_cast<const uint32_t*>(&req+1);
        printf("Set options %x on entry %p\n",o,input);
        const_cast<Ami::DescEntry&>(input->desc()).options(o);
      }
    }
    payload += req.size();
  }

  //
  //  Reconfigure Post Analyses whenever the PostAnalysis variable cache changes
  //    This guarantees that the post analyses are performed after the variables
  //      they depend upon are cached.
  //
  if (_features[PostAnalysis]->update())
    _srv.discover_post();

  _configured = true;
  pthread_cond_broadcast(&_condition);

  pthread_mutex_unlock(&_mutex);
}

void AnalysisFactory::analyze  ()
{
  pthread_mutex_lock(&_mutex);
  SummaryAnalysis::instance().analyze();
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    (*it)->analyze();
  }
  //
  //  Post-analyses go here
  //
  pthread_mutex_unlock(&_mutex);
}

void AnalysisFactory::remove(unsigned id)
{
  pthread_mutex_lock(&_mutex);
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
  pthread_mutex_unlock(&_mutex);
}
