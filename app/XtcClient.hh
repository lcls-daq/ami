#ifndef Ami_XtcClient_hh
#define Ami_XtcClient_hh

#include "pdsdata/xtc/XtcIterator.hh"
#include "ami/service/DumpSource.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "ami/app/EventFilter.hh"

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
  class EventFilter;
  class Entry;
  class NameService;
  class XtcCache;

  class XtcClient : private Pds::XtcIterator,
		    public DumpSource {
  public:
    XtcClient(std::vector<FeatureCache*>& cache, 
	      Factory&      factory, 
	      EventFilter&  filter,
	      bool          sync=false);
    ~XtcClient();
    void insert(EventHandler*);
    void remove(EventHandler*);
    void processDgram(Pds::Dgram*);
    void discover_wait();
  public:
    static XtcClient* instance();
    std::list<const EventHandler*> handlers() const;
  public:
    std::string dump() const;
  private:
    typedef std::list<EventHandler*> HList;
    typedef std::list<Composer*>     CList;
    typedef std::list<Entry*>        EList;
    int process(Pds::Xtc*);
    void _configure(Pds::Xtc* xtc, EventHandler* h);
    std::vector<FeatureCache*>& _cache;
    Factory&      _factory;
    EventFilter&  _filter;
    const Pds::Sequence* _seq;
    bool      _sync;
    HList     _handlers;
    CList     _composers;
    EList     _entry;
    bool      _ready;
    int       _ptime_index, _ptime_acc_index;    
    int       _pltnc_index;
    int       _event_index;
    int       _evfid_index;
    int       _evtim_index;
    int       _evrtm_index;
    int       _runno_index;
    double    _runno_value;
    Pds::ClockTime _runtim;
    unsigned  _nevents;
    bool      _recorded;
    NameService* _name_service;
    XtcCache*    _umap;
  };
}

#endif
