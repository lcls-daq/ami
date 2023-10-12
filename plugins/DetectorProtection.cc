#include "DetectorProtection.hh"

#include "ami/data/Cds.hh"

#include "ami/data/EntryScalar.hh"

#include "ami/app/NameService.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/alias.ddl.h"
#include "pdsdata/psddl/epics.ddl.h"

#include "ami/plugins/Protectors.hh"
#include "ami/plugins/ProtectionIOC.hh"

#include <cstdio>

#include "cadef.h"

namespace Ami {
 
};

using namespace Ami;

typedef std::map<unsigned, Protector*> ProtectorMap;
typedef ProtectorMap::iterator ProtectorIter;

DetectorProtection::DetectorProtection(const char* name, const char* short_name) :
  _cds(NULL),
  _lastTrip(Pds::ClockTime(0,0)),
  _fname(name),
  _sname(short_name),
  _alias_ready(false),
  _name_service(new NameService),
  _threshold(new Threshold("DetectorProtection.cfg", _name_service))
{
  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ),
           "detprotect calling ca_context_create" );
}

DetectorProtection::~DetectorProtection()
{
  for(ProtectorIter it = _dets.begin(); it != _dets.end(); ++it) {
    if (it->second) {
      delete it->second;
    }
  }
  _dets.clear();

  if (_name_service) {
    delete _name_service;
  }

  if (_threshold) {
    delete _threshold;
  }

  ca_context_destroy();
}

void DetectorProtection::reset(FeatureCache& f)
{
  for(ProtectorIter it = _dets.begin(); it != _dets.end(); ++it) {
    if (it->second) {
      if (_cds) {
        _cds->remove(it->second->entry());
      }
      delete it->second;
    }
  }
  _dets.clear();

  _threshold->reset();

  _alias_ready = false;
}

void DetectorProtection::clock(const Pds::ClockTime& clk)
{
  _clk=clk;
}

void DetectorProtection::configure(const Pds::ProcInfo&  src,
                                   const Pds::TypeId&    type,
                                   void*                 payload)
{
  // Name service for aliases
  if (type.id()==Pds::TypeId::Id_AliasConfig) {
    printf("%s found AliasConfig...", _sname);
    const Pds::Xtc* xtc = reinterpret_cast<const Pds::Xtc*>(payload)-1;
    if(xtc) {
      _name_service->append(*xtc);
      printf("(loaded name service)");
    } else {
      printf("(failed to load name service)");
    }
    printf("\n");

    _alias_ready = true;
    if (_cds) {
      for(ProtectorIter it = _dets.begin(); it != _dets.end(); ++it) {
        it->second->setName(_name_service->name(it->second->info()));
        EntryScalar* entry = it->second->entry();
        _cds->add(entry);
        entry->valid(_clk);
      }
    }
  }
}

void DetectorProtection::configure(const Pds::BldInfo&   src,
                                   const Pds::TypeId&    type,
                                   void*                 payload)
{}

void DetectorProtection::configure(const Pds::DetInfo&   src,
                                   const Pds::TypeId&    type,
                                   void*                 payload)
{
  if (type.id()==Pds::TypeId::Id_Epics) {
    _threshold->configure(src, type, payload);
  } else {
    Protector* prot = Protector::instance(src, type, payload, _threshold);
    if (prot) {
      _dets[src.phy()] = prot;
      if (_cds && _alias_ready) {
        prot->setName(_name_service->name(src));
        EntryScalar* entry = prot->entry();
        _cds->add(entry);
        entry->valid(_clk);
      }
      printf("%s initialized protection for %s\n",
             _sname,
             Pds::DetInfo::name(src));
    }
  }
}

const char* DetectorProtection::name() const
{
  return _fname;
}

void DetectorProtection::event(const Pds::BldInfo&  src,
                               const Pds::TypeId&   type,
                               const Pds::Damage&   dmg,
                               void*                payload)
{}

void DetectorProtection::event(const Pds::ProcInfo& src,
                               const Pds::TypeId&   type,
                               const Pds::Damage&   dmg,
                               void*                payload)
{}

void DetectorProtection::event(const Pds::DetInfo&  src,
                               const Pds::TypeId&   type,
                               const Pds::Damage&   dmg,
                               void*                payload)
{
  if ((dmg.value() & DMG_MASK) == 0) {
    if (type.id()==Pds::TypeId::Id_Epics) {
      const Pds::Epics::EpicsPvHeader* epics = reinterpret_cast<const Pds::Epics::EpicsPvHeader*>(payload);
      _threshold->event(epics);
    } else {
      ProtectorIter it = _dets.find(src.phy());
      if (it != _dets.end()) {
        it->second->event(type, payload);
      }
    }
  }
}

//
//  Remove all plot entries
//
void DetectorProtection::clear()
{
  if (_cds) {
    for(ProtectorIter it = _dets.begin(); it != _dets.end(); ++it) {
      if (it->second && it->second->hasEntry()) {
        _cds->remove(it->second->entry());
        // clean up the entry
        it->second->clear();
      }
    }
  }
  _cds = 0;
}

//
//  Create all plot entries
//
void DetectorProtection::create(Cds& cds)
{
  if (_alias_ready) {
    for(ProtectorIter it = _dets.begin(); it != _dets.end(); ++it) {
      it->second->setName(_name_service->name(it->second->info()));
      EntryScalar* entry = it->second->entry();
      cds.add(entry);
      entry->valid(_clk);
    }
  }
  _cds = &cds;
}

//
//  Analyze data for current event from the pointers we captured
//
bool DetectorProtection::accept()
{
  for(ProtectorIter it = _dets.begin(); it != _dets.end(); ++it) {
    it->second->accept(_clk);
  }
  return true;
}

//
//  Plug-in module creator
//

extern "C" UserModule* create() { return new DetectorProtection; }

extern "C" void destroy(UserModule* p) { delete p; }
