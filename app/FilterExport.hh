#ifndef Ami_FilterExport_hh
#define Ami_FilterExport_hh

#include "ami/data/ConfigureRequest.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <list>

namespace Pds {
  class Src;
};

namespace Ami {
  class AbsFilter;
  class Analysis;
  class EventHandler;

  class FilterImportCb {
  public:
    virtual ~FilterImportCb() {}
    virtual void handler (const Pds::Src&, 
                          const std::list<Pds::TypeId::Type>&,
                          const std::list<int>& signatures) = 0;
    virtual void analysis(unsigned id, 
                          ConfigureRequest::Source,
                          unsigned input, 
                          unsigned output,
                          void*    op) = 0;
    virtual void filter  (const AbsFilter&) = 0;
  };

  class FilterImport {
  public:
    FilterImport(const char* fname);
  public:
    const std::string& stream() const { return _stream; }
  public:
    void parse_handlers(FilterImportCb&);
    void parse_analyses(FilterImportCb&);
    void parse_filter  (FilterImportCb&);
  private:
    std::string _stream;
    AbsFilter*  _filter;
  };

  class FilterExport {
  public:
    FilterExport(const AbsFilter&, 
		 const std::list<const EventHandler*>&,
		 const std::list<const Analysis*>&);
    ~FilterExport();
  public:
    void write(const char*) const;
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
    typedef std::list<const Analysis*>     AList;
    typedef std::list<const EventHandler*> HList;
    AbsFilter*                     _filter;
    HList _handlers;
    AList _analyses;
  };
};

#endif
