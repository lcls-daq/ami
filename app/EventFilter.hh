#ifndef Ami_EventFilter_hh
#define Ami_EventFilter_hh

#include "pdsdata/xtc/XtcIterator.hh"

#include "ami/data/UserFilter.hh"
#include <list>

namespace Pds { class Dgram; };

namespace Ami {
  class EventFilter : public Pds::XtcIterator {
  public:
    EventFilter(std::list<UserFilter*>& filters) : _filters(filters) {}
  public:
    bool accept(Dgram*);
  public:
    int process(Xtc*);
  private:
    std::list<UserFilter*>& _filters;
  };
};

#endif
