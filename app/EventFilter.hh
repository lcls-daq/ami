#ifndef Ami_EventFilter_hh
#define Ami_EventFilter_hh

#include "pdsdata/xtc/XtcIterator.hh"

#include "ami/data/UserFilter.hh"
#include <list>
#include <vector>

namespace Pds { class Dgram; class Xtc; };

namespace Ami {
  class FeatureCache;

  class EventFilter : private Pds::XtcIterator {
  public:
    EventFilter(std::list<UserFilter*>& filters,
                FeatureCache&           cache);
    ~EventFilter();
  public:
    void enable      (unsigned);
    void configure   (const Xtc&);
    void add_to_cache();
    bool accept      (Dgram*);
  private:
    int process(Xtc*);
  private:
    std::list<UserFilter*>& _filters;
    FeatureCache&           _cache;
    std::vector<int>        _index;
    unsigned                _enable;
  };
};

#endif
