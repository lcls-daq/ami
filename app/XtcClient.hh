#ifndef Ami_XtcClient_hh
#define Ami_XtcClient_hh

#include "pdsdata/xtc/XtcIterator.hh"

#include <list>

namespace Pds {
  class Dgram;
  class Xtc;
  class Sequence;
};

namespace Ami {

  class FeatureCache;
  class Factory;
  class Composer;
  class EventHandler;
  class Entry;
  class UserAnalysis;

  class XtcClient : private XtcIterator {
  public:
    XtcClient(FeatureCache& cache, 
	      Factory&      factory, 
	      std::list<UserAnalysis*>& user,
	      bool          sync=false);
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
    typedef std::list<Composer*>     CList;
    typedef std::list<UserAnalysis*> UList;
    FeatureCache& _cache;
    Factory&      _factory;
    UList&        _user;
    const Pds::Sequence* _seq;
    bool      _sync;
    HList     _handlers;
    CList     _composers;
    Entry*    _entry;
    bool      _ready;
    int       _ptime_index;    
    int       _pltnc_index;
  };
}

#endif
