#include "DetectorProtection.hh"

#include "ami/data/Cds.hh"

#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"

#include "ami/app/NameService.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/psddl/alias.ddl.h"
#include "pdsdata/psddl/epics.ddl.h"

// its preprocesser does ugly things....
#include "ami/plugins/PVWriter.hh"

#include <fstream>
#include <sstream>
#include <string>
#include <time.h>

#include <sys/resource.h>

static const char* path = getenv("HOME");

namespace Ami {

  class PVHandler {
  public:
    PVHandler(const std::string& pvbase);
    virtual ~PVHandler();

    void configure(const char* pvname, int16_t pvid);

    void event(const Pds::Epics::EpicsPvHeader* epics);

    void reset();

    int32_t threshold() const { return _thres_value; }
    int32_t npixels() const { return _npixel_value; }
    bool enabled() const { return _enable_value; }

  private:
    std::string _pvbase;
    std::string _thres_pv;
    std::string _npixel_pv;
    std::string _enable_pv;
    std::string _shutter_pv;
    int16_t _thres_epics;
    int16_t _npixel_epics;
    int16_t _enable_epics;
    int32_t _thres_value;
    int32_t _npixel_value;
    bool    _enable_value;
  };
  
  class Protector {
  public:
    Protector(const Pds::DetInfo& info,
              const PVHandler* handler);
    virtual ~Protector();
    static Protector* instance(const Pds::DetInfo& info,
                               const Pds::TypeId& type,
                               void* payload,
                               const Threshold* threshold);

    const Pds::DetInfo& info() const;
    void setName(const char* name);
    void clear();
    bool hasEntry() const;
    EntryScalar* entry();

    virtual void event(const Pds::TypeId& type,
                       void* payload) = 0;

    virtual void analyzeDetector() = 0;

  private:
    std::string        _name;
    const Pds::DetInfo _info;
    EntryScalar*       _entry;
    const PVHandler*   _handler;
  };

  class CsPadProtector : public Protector {
  public:
    CsPadProtector(const Pds::DetInfo& info,
                   const Pds::TypeId&  type,
                   void*               payload,
                   const PVHandler*    handler);
    virtual ~CsPadProtector();

    virtual void event(const Pds::TypeId& type,
                       void* payload);
    
    virtual void analyzeDetector();
  };

  class Threshold {
  public:
    Threshold(const char* fname, NameService* name_service);
    virtual ~Threshold();

    void configure(const Pds::DetInfo&  src,
                   const Pds::TypeId&   type,
                   void*                payload);

    void event(const Pds::Epics::EpicsPvHeader* epics);

    void reset();

    const PVHandler* lookup(const Pds::DetInfo&  src,
                            const Pds::TypeId&   type) const;

  private:
    const char* _fname;
    NameService* _name_service;
    std::map<Pds::DetInfo, PVHandler*> _detpvs;
    std::map<std::string, PVHandler*>  _typepvs;
  };
};

using namespace Ami;

Protector::Protector(const Pds::DetInfo& info,
                     const PVHandler* handler) :
  _name(Pds::DetInfo::name(info)),
  _info(info),
  _entry(NULL),
  _handler(handler)
{}

Protector::~Protector()
{
  if (_entry) {
    delete _entry;
  }
}

Protector* Protector::instance(const Pds::DetInfo& info,
                               const Pds::TypeId& type,
                               void* payload,
                               const Threshold* threshold)
{
  Protector* prot = NULL;
  const PVHandler* handler = threshold->lookup(info, type);
  // if there is a threshold handler for detector run the protection
  if (handler) {
    if (type.id()==Pds::TypeId::Id_CspadConfig) {
      prot = new CsPadProtector(info, type, payload, handler);
    }  
  }
  return prot;
}

const Pds::DetInfo& Protector::info() const
{
  return _info;
}

void Protector::clear()
{
  if (_entry) {
    delete _entry;
    _entry = 0;
  }
}

void Protector::setName(const char* name)
{
  if (name) {
    _name.assign(name);
  }
}

bool Protector::hasEntry() const
{
  return _entry != NULL;
}

EntryScalar* Protector::entry()
{
  std::string title = "Pixels over threshold#" + _name;
  if (!_entry) {
    _entry = new EntryScalar(DescScalar(title.c_str(), "npixels"));
  }

  return _entry;
}

CsPadProtector::CsPadProtector(const Pds::DetInfo& info,
                               const Pds::TypeId&  type,
                               void*               payload,
                               const PVHandler*    handler) :
  Protector(info, handler)
{}

CsPadProtector::~CsPadProtector()
{}

void CsPadProtector::event(const Pds::TypeId& type,
                           void* payload)
{}


void CsPadProtector::analyzeDetector()
{}


typedef std::map<unsigned, Protector*> ProtectorMap;
typedef ProtectorMap::iterator ProtectorIter;
typedef std::map<std::string, PVHandler*> PvMap;
typedef PvMap::iterator PvIter;
typedef PvMap::const_iterator ConstPvIter;
typedef std::map<Pds::DetInfo, PVHandler*> DetPvMap;
typedef DetPvMap::iterator DetPvIter;
typedef DetPvMap::const_iterator ConstDetPvIter;

PVHandler::PVHandler(const std::string& pvbase) :
  _pvbase(pvbase),
  _thres_pv(_pvbase+":ADU"),
  _npixel_pv(_pvbase+":DPI"),
  _enable_pv(_pvbase+":ACTIVE"),
  _shutter_pv(_pvbase+":TRIP"),
  _thres_epics(-1),
  _npixel_epics(-1),
  _enable_epics(-1),
  _thres_value(0x100000),
  _npixel_value(0x4000),
  _enable_value(false)
{
  printf("PVs loaded: thres - %s, npixels - %s, enable - %s, tripper - %s\n",
         _thres_pv.c_str(), _npixel_pv.c_str(), _npixel_pv.c_str(), _shutter_pv.c_str());
}

PVHandler::~PVHandler()
{}

void PVHandler::configure(const char* pvname, int16_t pvid)
{
  if (_thres_pv.compare(pvname) == 0) {
    printf("Found PV for configuring tripCountThreshold: %s\n", pvname);
    _thres_epics = pvid;
  } else if (_npixel_pv.compare(pvname) == 0) {
    printf("Found PV for configuring nPixelsToTrip: %s\n", pvname);
    _npixel_epics = pvid;
  } else if (_enable_pv.compare(pvname) == 0) {
    printf("Found PV for configuring enableTrip: %s\n", pvname);
    _enable_epics = pvid;
  }
}

void PVHandler::event(const Pds::Epics::EpicsPvHeader* epics)
{
  if(epics->dbrType() == 19 /*Pds::Epics::DBR_TIME_LONG*/) {
    if(epics->pvId() == _thres_epics || epics->pvId() == _npixel_epics) {
      const Pds::Epics::EpicsPvTimeLong* pv = reinterpret_cast<const Pds::Epics::EpicsPvTimeLong*>(epics);
      if(epics->pvId() == _thres_epics) {
        _thres_value = pv->value(0);
        printf("%s is now equal to %d\n", _thres_pv.c_str(), _thres_value);
      } else if(epics->pvId() == _npixel_epics) {
        _npixel_value = pv->value(0);
        printf("%s is now equal to %d\n", _npixel_pv.c_str(), _npixel_value);
      }
    }
  } else if(epics->dbrType() == 17 /*Pds::Epics::DBR_TIME_ENUM*/) {
    if(epics->pvId() == _enable_epics) {
      const Pds::Epics::EpicsPvTimeEnum* pv = reinterpret_cast<const Pds::Epics::EpicsPvTimeEnum*>(epics);
      _enable_value = (pv->value(0) != 0);
      printf("%s is now equal to %s\n", _enable_pv.c_str(), _enable_value ? "true" : "false");
    }
  }
}

void PVHandler::reset()
{
  _thres_epics = -1;
  _npixel_epics = -1;
  _enable_epics = -1;
}

Threshold::Threshold(const char* fname, NameService* name_service) :
  _fname(fname),
  _name_service(name_service)
{
  char fpath[128];
  sprintf(fpath, _fname);
  std::string line;
  std::ifstream file;
  file.open(fpath);
  if(!file.good()) {
    printf("detprotect: unable to open %s\n",fpath);
    sprintf(fpath,"%s/%s", path, _fname);
    file.open(fpath);
  }
  if(file.good()) {
    while (std::getline(file, line)) {
      if(line[0]!='#') {
        std::stringstream ss(line);
        std::string key;
        std::string value;
        ss >> key >> value;
        Pds::DetInfo det(key.c_str());
        if (det.device() >= Pds::DetInfo::NumDevice || det.detector() >= Pds::DetInfo::NumDetector) {
          if (_typepvs.find(key) == _typepvs.end()) {
            _typepvs[key] = new PVHandler(value);
            printf("detprotect: PVs base names loaded from configuration file: %s - %s\n",
                   key.c_str(), value.c_str());
          }
        } else {
          if (_detpvs.find(det) == _detpvs.end()) {
            _detpvs[det] = new PVHandler(value);
            printf("detprotect: PVs base names loaded from configuration file: %s - %s\n",
                   Pds::DetInfo::name(det), value.c_str());
          }
        }
      }
    }
  } else {
    printf("detprotect: unable to open %s, use default values and disable shutter\n", fpath);
  }
}

Threshold::~Threshold()
{
  for (PvIter it = _typepvs.begin(); it != _typepvs.end(); ++it) {
    if (it->second) {
      delete it->second;
    }
  }
  _typepvs.clear();
  for (DetPvIter it = _detpvs.begin(); it != _detpvs.end(); ++it) {
    if (it->second) {
      delete it->second;
    }
  }
  _detpvs.clear();
}

void Threshold::configure(const Pds::DetInfo&  src,
                          const Pds::TypeId&   type,
                          void*                payload)
{
  const Pds::Epics::ConfigV1* temp_pv = reinterpret_cast<const Pds::Epics::ConfigV1*>(payload);
  Pds::Epics::PvConfigV1 pv_conf = temp_pv->getPvConfig()[0];
  int16_t temp_id = (int16_t) temp_pv->numPv();
  for (PvIter it = _typepvs.begin(); it != _typepvs.end(); ++it) {
    it->second->configure(pv_conf.description(), temp_id);
  }
  for (DetPvIter it = _detpvs.begin(); it != _detpvs.end(); ++it) {
    it->second->configure(pv_conf.description(), temp_id);
  }
}

void Threshold::event(const Pds::Epics::EpicsPvHeader* epics)
{
  for (PvIter it = _typepvs.begin(); it != _typepvs.end(); ++it) {
    it->second->event(epics);
  }
  for (DetPvIter it = _detpvs.begin(); it != _detpvs.end(); ++it) {
    it->second->event(epics);
  }
}

void Threshold::reset()
{
  for (PvIter it = _typepvs.begin(); it != _typepvs.end(); ++it) {
    if (it->second) {
      it->second->reset();
    }
  }
  for (DetPvIter it = _detpvs.begin(); it != _detpvs.end(); ++it) {
    if (it->second) {
      it->second->reset();
    }
  }
}

const PVHandler* Threshold::lookup(const Pds::DetInfo&  src,
                                   const Pds::TypeId&   type) const
{
  ConstPvIter it;
  ConstDetPvIter det_it;

  det_it = _detpvs.find(src);
  if (det_it != _detpvs.end()) {
    return det_it->second;
  }
    
  const char* alias = _name_service->name(src);
  if (alias) {
    it = _typepvs.find(alias);
    if (it != _typepvs.end()) {
      return it->second;
    }
  }

  std::string type_str = Pds::TypeId::name(type.id());
  it = _typepvs.find(type_str.substr(0, type_str.rfind("Config")));
  if (it != _typepvs.end()) {
    return it->second;
  }

  return NULL; 
}


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
    printf("%s found AliasConfig...",_sname);
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
      printf("Detector protection initialized for %s\n", Pds::DetInfo::name(src));
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
    it->second->analyzeDetector();
  }
  return true;
}

//
//  Plug-in module creator
//

extern "C" UserModule* create() { return new DetectorProtection; }

extern "C" void destroy(UserModule* p) { delete p; }
