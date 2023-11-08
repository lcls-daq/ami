#include "ProtectionIOC.hh"

#include "ami/app/NameService.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/alias.ddl.h"
#include "pdsdata/psddl/epics.ddl.h"

// its preprocesser does ugly things....
#include "ami/plugins/PVWriter.hh"

#include <fstream>
#include <sstream>

static const char* path = getenv("HOME");
static const char* pname = "DetProtect";

using namespace Ami;

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
  _enable_value(false),
  _shutter(NULL)
{
  printf("%s PVs loaded: thres - %s, npixels - %s, enable - %s, tripper - %s\n",
         pname,
         _thres_pv.c_str(),
         _npixel_pv.c_str(),
         _npixel_pv.c_str(),
         _shutter_pv.c_str());

  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ),
           "detprotect calling ca_context_create" );

  // needs to be created after context is initialized
  _shutter = new Ami_Epics::PVWriter(_shutter_pv.c_str());
}

PVHandler::~PVHandler()
{
  if (_shutter) {
    delete _shutter;
  }
}

void PVHandler::configure(const char* pvname, int16_t pvid)
{
  if (_thres_pv.compare(pvname) == 0) {
    printf("%s found PV for configuring tripCountThreshold: %s\n",
           pname, pvname);
    _thres_epics = pvid;
  } else if (_npixel_pv.compare(pvname) == 0) {
    printf("%s found PV for configuring nPixelsToTrip: %s\n",
           pname, pvname);
    _npixel_epics = pvid;
  } else if (_enable_pv.compare(pvname) == 0) {
    printf("%s found PV for configuring enableTrip: %s\n",
           pname, pvname);
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
      } else if(epics->pvId() == _npixel_epics) {
        _npixel_value = pv->value(0);
      }
    }
  } else if(epics->dbrType() == 17 /*Pds::Epics::DBR_TIME_ENUM*/) {
    if(epics->pvId() == _enable_epics) {
      const Pds::Epics::EpicsPvTimeEnum* pv = reinterpret_cast<const Pds::Epics::EpicsPvTimeEnum*>(epics);
      _enable_value = (pv->value(0) != 0);
    }
  }
}

void PVHandler::trip() const
{
  *(dbr_enum_t*) _shutter->data() = 0;
  _shutter->put();
  ca_flush_io();
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
    printf("%s unable to open %s\n", pname, fpath);
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
            printf("%s PVs base names loaded from configuration file: %s - %s\n",
                   pname, key.c_str(), value.c_str());
          }
        } else {
          if (_detpvs.find(det) == _detpvs.end()) {
            _detpvs[det] = new PVHandler(value);
            printf("%s PVs base names loaded from configuration file: %s - %s\n",
                   pname, Pds::DetInfo::name(det), value.c_str());
          }
        }
      }
    }
  } else {
    printf("%s unable to open %s, use default values and disable shutter\n", pname, fpath);
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
