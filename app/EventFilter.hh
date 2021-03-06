#ifndef Ami_EventFilter_hh
#define Ami_EventFilter_hh

#include "pdsdata/xtc/XtcIterator.hh"

#include "ami/data/UserModule.hh"
#include <list>
#include <vector>

namespace Pds { class Dgram; class Xtc; class Sequence; };

namespace Ami {
  class AbsFilter;
  class ConfigureRequest;
  class FeatureCache;
  class XtcCache;

  class Analysis;
  class EventHandler;

  class EventFilter : private Pds::XtcIterator {
  public:
    EventFilter(std::list<UserModule*>& filters,
                FeatureCache&           cache);
    ~EventFilter();
  public:
    void reset       ();
    void enable      (const ConfigureRequest&,
		      const std::list<const Analysis*>&,
		      const std::list<const EventHandler*>&);
    void configure   (Pds::Dgram*);
    bool accept      (Pds::Dgram*,XtcCache&);
    bool accept      ();
  public:
    const std::list<UserModule*>& modules() const { return _filters; }
  private:
    int process(Pds::Xtc*);
  private:
    std::list<UserModule*>& _filters;
    FeatureCache&           _cache;
    const AbsFilter*        _f;
    unsigned                _enable;
    const Pds::Sequence*    _seq;
    XtcCache*               _xcache;
  };
};

#endif
