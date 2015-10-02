#include "AnalysisFactory.hh"

#include "ami/app/EventFilter.hh"
#include "ami/app/XtcClient.hh"

#include "ami/data/UserModule.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/Message.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/RefreshRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/server/ServerManager.hh"

#include <sstream>
#include <stdlib.h>

//#define DBUG

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
  _user_cds  (user.size()),
  _filter    (filter),
  _waitingForConfigure(false)
{
  pthread_mutex_init(&_mutex, NULL);
  pthread_cond_init(&_condition, NULL);

  unsigned i=0;
  for(UList::iterator it=user.begin(); it!=user.end(); it++,i++) {
    _user_cds[i] = 0;
    (*it)->driver = this;
  }
}

AnalysisFactory::~AnalysisFactory()
{
  for(unsigned i=0; i<_user_cds.size(); i++)
    if (_user_cds[i])
      delete _user_cds[i];
}

std::vector<FeatureCache*>& AnalysisFactory::features() { return _features; }

Cds& AnalysisFactory::discovery() { return _cds; }
Cds& AnalysisFactory::hidden   () { return _ocds; }

static void remove_from_list(AnList& alist, unsigned id)
{
  AnList newlist;
  for(AnList::iterator it=alist.begin(); it!=alist.end(); it++) {
    Analysis* a = *it;
    if (a->id() == id) {
      delete a;
    }
    else {
      a->use();
      newlist.push_back(a);
    }
  }
  alist = newlist;
}

static void delete_list(AnList& alist)
{
  for(AnList::iterator it=alist.begin(); it!=alist.end(); it++) {
    Analysis* an = *it;
    delete an;
  }
  alist.clear();
}

static void dump_list(AnList& alist)
{
  for(AnList::iterator it=alist.begin(); it!=alist.end(); it++) {
    const DescEntry& desc = (*it)->output();
    printf("Analysis %2d : [%2d][%s] %s\n", 
           (*it)->id(), 
           desc.signature(),
	   DescEntry::type_str(desc.type()), 
           desc.name());
  }
}

static AnList extract_list(AnList& srcList,
                           unsigned id)
{
  AnList newlist;
  AnList oldlist;
  for(AnList::iterator it=srcList.begin(); it!=srcList.end(); it++) {
    Analysis* a = *it;
    if (a->id() == id)
      oldlist.push_back(a);
    else {
      a->use();
      newlist.push_back(a);
    }
  }
  srcList = newlist;
  return oldlist;
}

//
//  The discovery cds has changed
//
void AnalysisFactory::discover(bool waitForConfigure) 
{
  pthread_mutex_lock(&_mutex);
  delete_list(_analyses);
  delete_list(_post_analyses);
  
  if (waitForConfigure) {
    if (_waitingForConfigure) {
      printf("Already waiting for configure?\n");
      *((char *)0) = 0;
    }
    _waitingForConfigure = true;
  }

  _features[PostAnalysis]->update();  // clear the update flag
  _srv.discover(); 
  pthread_mutex_unlock(&_mutex);

  printf("AnalysisFactory::discover(%c) complete\n",waitForConfigure?'T':'F');
}

void AnalysisFactory::beginRun(unsigned v)
{ _srv.beginRun(v); }

void AnalysisFactory::endRun(unsigned v)
{ _srv.endRun(v); }

void AnalysisFactory::discover_wait()
{
  pthread_mutex_lock(&_mutex);
  if (_waitingForConfigure) {
    printf("AnalysisFactory: waiting for configuration...\n");
    for (;;) {
      if (! _waitingForConfigure) {
        break;
      }
      pthread_cond_wait(&_condition, &_mutex);
    }
    printf("AnalysisFactory: done waiting for configuration.\n");
  }
  pthread_mutex_unlock(&_mutex);
  printf("AnalysisFactory::discover_wait complete\n");
}

void AnalysisFactory::configure(unsigned       id,
				const Message& msg, 
				const char*    payload, 
				Cds&           cds,
                                bool           post_svc)
{
  pthread_mutex_lock(&_mutex);

  _cds.clear_used();
  _features[PreAnalysis ]->clear_used();
  _features[PostAnalysis]->clear_used();

  //
  //  Segregate the entries belonging to the configuring client
  //
  AnList& srcList = post_svc ? _post_analyses : _analyses;
  AnList& othList = post_svc ? _analyses : _post_analyses;
  extract_list(othList, id); // update "used"
  AnList oldList = extract_list(srcList, id);

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

#ifdef DBUG
    printf("AnalysisFactory::configure request {%d:%d:%d:%d:%d:%d}\n",
	   req.state(),req.source(),req.scalars(),req.input(),req.output(),req.size());
#endif

    if (req.source() == ConfigureRequest::User) {
      int i=0;
      for(UList::iterator it=_user.begin(); it!=_user.end(); it++,i++) {
        if (req.input() == i) {
	  _srv.register_key(i,id);
          Cds* ucds = _user_cds[i];

          switch(req.state()) {
          case ConfigureRequest::Destroy:
            if (ucds) ucds->reset_plots();
            break;
          case ConfigureRequest::Create:
            if (ucds==0) {
              _user_cds[i] = ucds = new Cds((*it)->name());
              (*it)->clear();
              (*it)->create(*ucds);
            }
            ucds->mirror(cds);
            break;
          default:
            break;
          }

          break;
        }
      }
    }
    else if (req.source() == ConfigureRequest::Filter) {
      std::list<const Analysis*> a;
      for(AnList::const_iterator it=srcList.begin();
	  it!=srcList.end(); it++)
	a.push_back(*it);
      for(AnList::const_iterator it=othList.begin();
	  it!=othList.end(); it++)
	a.push_back(*it);
      _filter.enable(req, a, XtcClient::instance()->handlers());
    }
    else {
      const Cds* pcds = 0;
      const char *reqType = NULL;
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
      if (req.state() == ConfigureRequest::Reset) {
	cds.reset_plots();
        for(AnList::iterator it=oldList.begin(); it!=oldList.end(); it++)
	  (*it)->use();
	srcList.splice(srcList.end(),oldList);
      }
      else if (!input) {
        printf("AnalysisFactory::configure failed (null input) for ConfigureRequest::%s\n", reqType);
        printf("\tinp %d  out %d  size %d\n",req.input(),req.output(),req.size());
        input_cds.showentries();
        if (req.input()>0)
          abort();
      }
      else if (req.state()==ConfigureRequest::Create) {
        //
        //  If the output entry already exists, don't need to create it.
        //
        bool lFound=false;
        for(AnList::iterator it=oldList.begin(); it!=oldList.end(); it++) {
          Analysis* a = *it;
          if (a->output().signature()==req.output() && a->valid()) {
#ifdef DBUG
            printf("Preserving analysis [%d%s] for %s [%d]\n",
		   id, post_svc ? ":Post":"",
                   a->output().name(),
                   a->output().signature());
#endif
	    a->use();
            srcList.push_back(a);
            oldList.remove(a);
            lFound=true;
            break;
          }
        }
        if (!lFound) {
          const char*  p     = reinterpret_cast<const char*>(&req+1);
          Analysis* a = new Analysis(id, *input, req.output(), cds, 
				     *_features[req.scalars()], 
				     *_features[Ami::PostAnalysis],
				     p);
	  a->use();
          srcList.push_back(a);
#ifdef DBUG
          printf("Created analysis [%d%s] for %s [%d] features %d\n",
		 id, post_svc ? ":Post":"",
                 a->output().name(),
                 a->output().signature(),
		 req.scalars());
#endif
        }
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
  //  Clean up any old client entries that were not requested
  //
  for(AnList::iterator it=oldList.begin(); it!=oldList.end(); it++) {
    Analysis* a = *it;
#ifdef DBUG
          printf("Removing analysis [%d%s] for %s [%d]\n",
		 id, post_svc ? ":Post":"",
                 a->output().name(),
                 a->output().signature());
#endif
    delete a;
  }

  _features[PreAnalysis ]->use(*_features[PostAnalysis]);

#ifdef DBUG
  cds.showentries();
#endif

  printf("===\n");
  dump_list(_analyses);
  dump_list(_post_analyses);
  printf("===\n");

  //
  //  Reconfigure Post Analyses whenever the PostAnalysis variable cache changes
  //    This guarantees that the post analyses are performed after the variables
  //      they depend upon are cached.
  //
  if (_features[PostAnalysis]->update()) {
    printf("PostAnalysis updated\n");
    _srv.discover_post();
  }

  if (_waitingForConfigure) {
    _waitingForConfigure = false;
    pthread_cond_signal(&_condition);
    printf("AnalysisFactory: configuration is done.\n");
  }

  pthread_mutex_unlock(&_mutex);
}

void AnalysisFactory::refresh(unsigned       id,
                              const Message& msg, 
                              const char*    payload, 
                              Cds&           cds)
{
  pthread_mutex_lock(&_mutex);

  // create
  const char* const end = payload + msg.payload();
  while(payload < end) {
    const RefreshRequest& req = *reinterpret_cast<const RefreshRequest*>(payload);
    
    if (req.source() == ConfigureRequest::User) {
    }
    else if (req.source() == ConfigureRequest::Filter) {
    }
    else {
      Cds* pcds = 0;
      const char *reqType = NULL;
      switch(req.source()) {
        case ConfigureRequest::Discovery: pcds = &_cds; reqType = "Discovery"; break;
        case ConfigureRequest::Analysis : pcds = & cds; reqType = "Analysis"; break;
        case ConfigureRequest::Hidden   : pcds = &_ocds; reqType = "Hidden"; break;
        default:
          printf("AnalysisFactory::refresh unknown source RefreshRequest::%d\n",req.source());
          break;
      }
      if (!pcds) {
        printf("AnalysisFactory::refresh failed (null pcds) for RefreshRequest::%s\n", reqType);
        printf("\tinp %d\n",req.input());
        break;
      }
      Cds& input_cds = *pcds;
      Entry* input = input_cds.entry(req.input());
      if (!input) {
        printf("AnalysisFactory::refresh failed (null input) for RefreshRequest::%s\n", reqType);
        printf("\tinp %d\n",req.input());
        input_cds.showentries();
      }
      else if (input->valid() && req.tmo().asDouble()>input->first()) {
        input->reset();
      }
    }
    payload += sizeof(req);
  }

  pthread_mutex_unlock(&_mutex);
}

void AnalysisFactory::analyze  ()
{
  pthread_mutex_lock(&_mutex);
  for(AnList::iterator it=_analyses.begin(); it!=_analyses.end(); it++) {
    (*it)->analyze();
  }
  //
  //  Post-analyses go here
  //
  for(AnList::iterator it=_post_analyses.begin(); it!=_post_analyses.end(); it++) {
    (*it)->analyze();
  }
  pthread_mutex_unlock(&_mutex);
}


void AnalysisFactory::remove(unsigned id)
{
  pthread_mutex_lock(&_mutex);

  _cds.clear_used();
  remove_from_list(_analyses,id);
  remove_from_list(_post_analyses,id);

  pthread_mutex_unlock(&_mutex);
}

void AnalysisFactory::recreate(UserModule* user)
{
  int i=0;
  for(UList::iterator it=_user.begin(); it!=_user.end(); it++,i++)
    if (user == *it) {
      _user_cds[i] = 0;
      _srv.discover_key(i);
      break;
    }
}

std::string AnalysisFactory::dump() const
{
  std::ostringstream s;
  s << "\tAnalysisFactory" << std::endl;
  s << _cds.dump();
  s << _ocds.dump();
  return s.str();
}
