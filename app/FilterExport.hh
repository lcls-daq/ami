#ifndef Ami_FilterExport_hh
#define Ami_FilterExport_hh

#include "pdsdata/xtc/Src.hh"

#include <list>

namespace Ami {
  class AbsFilter;
  class Analysis;
  class EventHandler;

  class FilterExport {
  public:
    FilterExport(const char*);
    FilterExport(const AbsFilter&, 
		 const std::list<const EventHandler*>&,
		 const std::list<const Analysis*>&);
    ~FilterExport();
  public:
    void write(const char*) const;
  public:
    const std::list<const EventHandler*>& handlers() const { return _handlers; }
    const std::list<const Analysis*>& analyses() const { return _analyses; }
  private:
    void _find_filter_sources(const AbsFilter&, 
			      const std::list<const EventHandler*>&,
			      const std::list<const Analysis*>&);
    const Analysis* _find_analysis_source(const Analysis*,
					  const std::list<const EventHandler*>&,
					  const std::list<const Analysis*>&);
    bool _from_discovery(const Analysis*) const;
    void _writex(char*&) const;
  private:
    AbsFilter*                     _filter;
    std::list<const EventHandler*> _handlers;
    std::list<const Analysis*    > _analyses;
  };
};

#endif
