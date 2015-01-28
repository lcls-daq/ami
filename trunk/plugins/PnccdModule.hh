#ifndef PnccdModule_hh
#define PnccdModule_hh


#include "ami/data/UserModule.hh"  // Plug-in module interface definition

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/pnccd.ddl.h"
#include "ndarray/ndarray.h"

namespace Ami {
  class Cds;         // Plot data service
  class EntryTH1F;   // Distribution plot
  class EntryProf;   // Distribution plot
  class EntryImage;   // Distribution plot
  class EntryTH2F;

  class PnccdModule : public UserModule {
  public:
    PnccdModule();
    ~PnccdModule();
  public:  // Handler functions
    void reset    (Ami::FeatureCache&);
    void clock    (const Pds::ClockTime& clk);     // timestamp of current event/configure
    void configure(const Pds::DetInfo&   src,      // configuration data callback
		   const Pds::TypeId&    type,
		   void*                 payload);
    void event    (const Pds::DetInfo&   src,      // event data callback
		   const Pds::TypeId&    type,
                   const Pds::Damage&    damage,
		   void*                 payload);
  public:  // Analysis functions
    void clear    ();         // remove Entry's
    void create   (Cds& cds); // create Entry's
    void analyze  ();         // fake
    const char* name() const;
    bool accept ();

  private:
    //  Resulting Plots
    Ami::Cds* _cds;
    Pds::ClockTime _clk;
    class ModulePlots;
    std::list<ModulePlots*> _plots;
    bool _changed;

  public:
    /// Empty instanciations
    void configure(const Pds::BldInfo&   src,      // configuration data callback
		   const Pds::TypeId&    type,
		   void*                 payload) {}
    void configure(const Pds::ProcInfo&  src,      // configuration data callback
		   const Pds::TypeId&    type,
		   void*                 payload) {}
    void event    (const Pds::BldInfo&   src,      // event data callback
		   const Pds::TypeId&    type,
		   const Pds::Damage&    damage,
		   void*                 payload) {}
    void event    (const Pds::ProcInfo&  src,      // event data callback
		   const Pds::TypeId&    type,
		   const Pds::Damage&    damage,
		   void*                 payload) {}

  };

};

#endif
