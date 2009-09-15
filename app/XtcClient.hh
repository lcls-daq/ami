#ifndef Ami_XtcClient_hh
#define Ami_XtcClient_hh

#include "ami/app/XtcMonitorClient.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include <list>

namespace Pds {
  class Xtc;
  class Sequence;
};

namespace Ami {

  class FeatureCache;
  class Factory;
  class EventHandler;

  class XtcClient : public XtcMonitorClient,
		    private XtcIterator {
  public:
    XtcClient(char* tag,
	      FeatureCache& cache, 
	      Factory& factory, 
	      bool sync=false);
    ~XtcClient();
  public:
    void insert(EventHandler*);
    void remove(EventHandler*);
  public:
    void processDgram(Pds::Dgram*);
  private:
    int  process(Pds::Xtc*);
  private:
    typedef std::list<EventHandler*> HList;
    FeatureCache& _cache;
    Factory&  _factory;
    const Pds::Sequence* _seq;
    bool      _sync;
    HList     _handlers;
  };
}

#endif
