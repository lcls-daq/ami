#ifndef UserModule_hh
#define UserModule_hh

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Damage.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/UserModuleDriver.hh"

namespace Ami {
  class UserModule {
  public:
    virtual ~UserModule() {}
  public:  // Handler functions
    virtual void reset    (FeatureCache&) = 0;
    virtual void clock    (const Pds::ClockTime& clk) = 0;
    virtual void configure(const Pds::DetInfo&   src,
			   const Pds::TypeId&    type,
			   void*                 payload) = 0;
    virtual void configure(const Pds::BldInfo&   src,
			   const Pds::TypeId&    type,
			   void*                 payload) = 0;
    virtual void configure(const Pds::ProcInfo&  src,
			   const Pds::TypeId&    type,
			   void*                 payload) = 0;
    virtual void event    (const Pds::DetInfo&   src,
			   const Pds::TypeId&    type,
			   const Pds::Damage&    damage,
			   void*                 payload) = 0;
    virtual void event    (const Pds::BldInfo&   src,
			   const Pds::TypeId&    type,
			   const Pds::Damage&    damage,
			   void*                 payload) = 0;
    virtual void event    (const Pds::ProcInfo&  src,
			   const Pds::TypeId&    type,
			   const Pds::Damage&    damage,
			   void*                 payload) = 0;
    virtual bool uses     (const Pds::DetInfo&   src) { return true; }
    virtual bool uses     (const Pds::BldInfo&   src) { return true; }
    virtual bool uses     (const Pds::ProcInfo&  src) { return true; }
  public:
    virtual const char* name() const = 0;
    virtual bool accept () { return true; }
  public:
    virtual void clear (    ) = 0;
    virtual void create(Cds&) = 0;
  protected:
    void recreate() { driver->recreate(this); }
  public:
    UserModuleDriver* driver;
  };
};

typedef Ami::UserModule* create_m();
typedef void destroy_m(Ami::UserModule*);

#endif
