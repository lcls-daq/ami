#ifndef Ami_L3TModule_hh
#define Ami_L3TModule_hh

/**
 *  A sample module that filters based upon the EVR information 
 *  in the event.
 */

#include "pdsdata/app/L3FilterModule.hh"

#include "ami/app/FilterExport.hh"

#include "ami/data/Cds.hh"

#include <vector>
#include <map>

namespace Ami {
  class FeatureCache;
  class Analysis;
  class EventHandler;
  class NameService;

  namespace L3T {
    class L3TModule : public Pds::L3FilterModule,
                      public Ami::FilterImportCb {
    public:
      L3TModule();
      ~L3TModule();
    public:
      void pre_configure();
      void post_configure();
      void configure(const Pds::DetInfo&   src,
                     const Pds::TypeId&    type,
                     void*                 payload);
      void configure(const Pds::BldInfo&   src,
                     const Pds::TypeId&    type,
                     void*                 payload);
      void configure(const Pds::ProcInfo&  src,
                     const Pds::TypeId&    type,
                     void*                 payload);

      void pre_event();
      void event    (const Pds::DetInfo&   src,
                     const Pds::TypeId&    type,
                     void*                 payload);
      void event    (const Pds::BldInfo&   src,
                     const Pds::TypeId&    type,
                     void*                 payload);
      void event    (const Pds::ProcInfo&  src,
                     const Pds::TypeId&    type,
                     void*                 payload);
    private:   
      void _configure(const Pds::Src&      src,
                      const Pds::TypeId&   type,
                      void*                payload);
      void _event    (const Pds::Src&      src,
                      const Pds::TypeId&   type,
                      void*                payload);
    public:
      std::string name() const;
      std::string configuration() const;
      bool complete ();
      bool accept ();
    public:
      void handler (const Pds::Src&, 
                    const std::list<Pds::TypeId::Type>&,
                    const std::list<int>& signatures);
      void analysis(unsigned id, 
                    ConfigureRequest::Source,
                    unsigned input, 
                    unsigned output,
                    void*    op);
      void filter  (const AbsFilter&);
    private:
      FilterImport*              _import;
      std::vector<FeatureCache*> _features;
      std::list  <EventHandler*> _handlers;
      NameService*               _name_service;

      std::list<Analysis*>       _analyses;
      Cds                        _discovery;
      std::map<int,Cds*>         _cds;
      std::map<uint32_t,std::list<int> > _signatures;

      AbsFilter*                 _filter;
      bool                       _config_complete;
    };
  };
};
 
#endif
