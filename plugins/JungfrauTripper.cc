#include "JungfrauTripper.hh"

#include "ami/data/Cds.hh"

#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"

#include "ami/app/NameService.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/jungfrau.ddl.h"
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

using namespace Ami;

JungfrauTripper::JungfrauTripper(const char* name, const char* short_name) :
  _cds(0),
  _detsFound(0),
  _lastTrip(Pds::ClockTime(0,0)),
  _thres_epics(-1),
  _npixel_epics(-1),
  _enable_epics(-1),
  _fname(name),
  _sname(short_name),
  _thres_pv(0),
  _npixel_pv(0),
  _enable_pv(0),
  _shutter_pv(0),
  _name_service(new NameService),
  _shutter(0)
{
  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ),
           "jungfrautripper calling ca_context_create" );

  for (unsigned i=0;i<MaxDetsAllowed;i++) {
    _frame[i]=0;
    _config[i]=0;
    _dets[i]=0;
    _config_size[i]=0;
  }
  _shutter_pv = new char[64];

  char fpath[128];
  sprintf(fpath,"JungfrauTripperPVs.lst");
  std::string line;
  std::ifstream file;
  file.open(fpath);
  if(!file.good()) {
    printf("tripper::configure unable to open %s\n",fpath);
    sprintf(fpath,"%s/JungfrauTripperPVs.lst",path);
    file.open(fpath);
  }
  if(file.good()) {
    while (std::getline(file, line)) {
      if(line[0]!='#') {
        _thres_pv = new char[line.length() + strlen(":ADU") + 1];
        sprintf(_thres_pv, "%s:ADU", line.c_str());
        _npixel_pv = new char[line.length() + strlen(":DPI") + 1];
        sprintf(_npixel_pv, "%s:DPI", line.c_str());
        _enable_pv = new char[line.length() + strlen(":ACTIVE") + 1];
        sprintf(_enable_pv, "%s:ACTIVE", line.c_str());
        _shutter_pv = new char[line.length() + strlen(":TRIP") + 1];
        sprintf(_shutter_pv, "%s:TRIP", line.c_str());
        break;
      }
    }
    printf("PVs loaded from configuration file: thres - %s, npixels - %s, enable - %s, shutter_name - %s\n",
           _thres_pv,_npixel_pv,_enable_pv,_shutter_pv);
  } else {
    printf("tripper::configure unable to open %s, use default values and disable shutter\n",fpath);
  }
  if(_shutter_pv && strlen(_shutter_pv)) {
    _shutter = new Ami_Epics::PVWriter(_shutter_pv);
  }
  file.close();
}

JungfrauTripper::~JungfrauTripper()
{
  if (_name_service) delete   _name_service;
  if (_shutter)     delete    _shutter;
  if (_thres_pv)    delete[]  _thres_pv;
  if (_npixel_pv)   delete[]  _npixel_pv;
  if (_enable_pv)   delete[]  _enable_pv;
  if (_shutter_pv)  delete[]  _shutter_pv;

  ca_context_destroy();
}

void JungfrauTripper::reset    (FeatureCache& f)
{
  for (unsigned id=0; id<_detsFound; id++) {
    _config[id]=0;
    _config_size[id]=0;
    _dets[id]=0;
    if(_cds) _cds->remove(_result[id]);
  }
  _detsFound=0;
}
void JungfrauTripper::clock    (const Pds::ClockTime& clk) { _clk=clk; }
void JungfrauTripper::configure(const Pds::ProcInfo&    src,
                                const Pds::TypeId&    type,
                                void*                 payload){}
void JungfrauTripper::configure(const Pds::BldInfo&    src,
                                const Pds::TypeId&    type,
                                void*                 payload){}
void JungfrauTripper::configure(const Pds::DetInfo&    src,
                                const Pds::TypeId&    type,
                                void*                 payload) 
{
  if (type.id()==Pds::TypeId::Id_JungfrauConfig) {

    _nevt   = 0;

    // need to copy the configuration, since it doesn't persist
    const Pds::Jungfrau::ConfigV3* orig_conf = reinterpret_cast<const Pds::Jungfrau::ConfigV3*>(payload);
    if (orig_conf->_sizeof() > _config_size[_detsFound]) {
      // need to allocate a larger buffer
      if (_config[_detsFound]) delete[] (char *)(_config[_detsFound]);
      char* buffer = new char[orig_conf->_sizeof()];
      _config[_detsFound] = reinterpret_cast<Pds::Jungfrau::ConfigV3*>(buffer);
    }
    _config_size[_detsFound] = orig_conf->_sizeof();
    memcpy(_config[_detsFound], orig_conf, orig_conf->_sizeof());
      
    _dets[_detsFound] = src.phy();

    _nPixelsToTrip[_detsFound] = 0x100000;
    _tripCountThreshold[_detsFound] = 0x4000;
    _enableTrip[_detsFound] = false;
    if(_cds) {
      char title[80];
      sprintf(title, "Pixels over threshold, det 0x%x#Tripper", _dets[_detsFound]);
      _result[_detsFound] = new EntryScalar(DescScalar(title, title));
      _cds->add(_result[_detsFound]);
      _result[_detsFound]->valid(_clk);
    }
    _detsFound++;
  } else if(type.id()==Pds::TypeId::Id_Epics) {
    const Pds::Epics::ConfigV1* temp_pv = reinterpret_cast<const Pds::Epics::ConfigV1*>(payload);
    Pds::Epics::PvConfigV1 pv_conf = temp_pv->getPvConfig()[0];
    int16_t temp_id = (int16_t) temp_pv->numPv();
    if(_thres_pv && strcmp(pv_conf.description(),_thres_pv)==0) {
      printf("Found PV for configuring tripCountThreshold: %s\n", pv_conf.description());
      _thres_config = temp_pv;
      _thres_epics = temp_id;
    } else if(_npixel_pv && strcmp(pv_conf.description(),_npixel_pv)==0) {
      printf("Found PV for configuring nPixelsToTrip: %s\n", pv_conf.description());
      _npixel_config = temp_pv;
      _npixel_epics = temp_id;
    } else if(_enable_pv && strcmp(pv_conf.description(),_enable_pv)==0) {
      printf("Found PV for configuring enableTrip: %s\n", pv_conf.description());
      _enable_config = temp_pv;
      _enable_epics = temp_id;
    }
  }
}

const char* JungfrauTripper::name() const
{
  return _fname;
}


void JungfrauTripper::event    (const Pds::BldInfo&    src,
                                const Pds::TypeId&    type,
                                const Pds::Damage&     dmg,
                                void*                 payload)
{}

void JungfrauTripper::event    (const Pds::ProcInfo&    src,
                                const Pds::TypeId&    type,
                                const Pds::Damage&     dmg,
                                void*                 payload)
{}

//
//  Capture pointer to detector data we want
//
void JungfrauTripper::event    (const Pds::DetInfo&    src,
                                const Pds::TypeId&    type,
                                const Pds::Damage&     dmg,
                                void*                 payload) 
{
  if (type.id()==Pds::TypeId::Id_JungfrauElement) {
    if ((dmg.value() & DMG_MASK) == 0) {
      for (unsigned id=0; id<_detsFound; id++) {
        if (src.phy()==_dets[id]) _frame[id] = reinterpret_cast<const Pds::Jungfrau::ElementV2*>(payload);
      }
    }
  } else if (type.id()==Pds::TypeId::Id_Epics) {
    if ((dmg.value() & DMG_MASK) == 0) {
      const Pds::Epics::EpicsPvHeader* epics = reinterpret_cast<const Pds::Epics::EpicsPvHeader*>(payload);
      if(epics->dbrType() == 19 /*Pds::Epics::DBR_TIME_LONG*/) {
        if(epics->pvId() == _thres_epics || epics->pvId() == _npixel_epics) {
          const Pds::Epics::EpicsPvTimeLong* pv = reinterpret_cast<const Pds::Epics::EpicsPvTimeLong*>(epics);
          int32_t new_val = pv->value(0);
          if(epics->pvId() == _thres_epics) {
            for (unsigned id=0; id<_detsFound; id++) {
              if(_tripCountThreshold[id] != new_val) {
                printf("Updating trip count threshold value for det %d from %d to %d\n", id, _tripCountThreshold[id], new_val);
                _tripCountThreshold[id] = new_val;
              }
            }
          } else if(epics->pvId() == _npixel_epics) {
            for (unsigned id=0; id<_detsFound; id++) {
              if(_nPixelsToTrip[id] != new_val) {
                printf("Updating number of pixels for trip for det %d from %d to %d\n", id, _nPixelsToTrip[id], new_val);
                _nPixelsToTrip[id] = (unsigned) pv->value(0);
              }
            }
          }
        }
      } else if(epics->dbrType() == 17 /*Pds::Epics::DBR_TIME_ENUM*/) {
        if(epics->pvId() == _enable_epics) {
          const Pds::Epics::EpicsPvTimeEnum* pv = reinterpret_cast<const Pds::Epics::EpicsPvTimeEnum*>(epics);
          bool new_val = (pv->value(0) != 0);
          for (unsigned id=0; id<_detsFound; id++) {
            if(_enableTrip[id] != new_val) {
              printf("Updating trip enabled state for det %d from %d to %d\n", id, _enableTrip[id], new_val);
              _enableTrip[id] = new_val;
            }
          }
        }
      }
    }
  }
}

//
//  Remove all plot entries
//

void JungfrauTripper::clear    () 
{
  if (_cds) {
    for (unsigned id=0; id<_detsFound; id++) {
      _cds->remove(_result[id]);
    }
  }
  _cds = 0;
}

//
//  Create all plot entries
//

void JungfrauTripper::create   (Cds& cds)
{
  _cds = &cds; 
  for (unsigned id=0; id<_detsFound; id++) {
    char title[80];
    sprintf(title, "Pixels over threshold, det 0x%x#Tripper", _dets[id]);
    _result[id] = new EntryScalar(DescScalar(title, title));
    cds.add(_result[id]);
    _result[id]->valid(_clk);
  }
}

//
//  Analyze data for current event from the pointers we captured
//
bool JungfrauTripper::accept  () 
{
  analyzeDetector();
  return true;
}

void JungfrauTripper::analyzeDetector  () 
{
  // Note that the way _frame is set ensures that multiple jfs
  // always show up in the same order in the _frame array.  this
  // ensures we will use the correct bad pixel map with multiple
  // detectors. - cpo

  for(unsigned currentDet=0;currentDet<_detsFound;currentDet++) {
    if (!_frame[currentDet]) continue;

    bool trip = false;
    int32_t pixelCount = 0;

    const Pds::Jungfrau::ElementV2& s = *_frame[currentDet];
    const Pds::Jungfrau::ConfigV3& cfg = *_config[currentDet];

    ndarray<const uint16_t, 3> data = s.frame(cfg);
    for (unsigned i=0; i<data.shape()[0]; i++) {
      for (unsigned j=0; j<data.shape()[1]; j++) {
        for (unsigned k=0; k<data.shape()[2]; k++) {
          if (data(i,j,k) > _tripCountThreshold[currentDet]) {
            if(++pixelCount > _nPixelsToTrip[currentDet]) {
              trip = true;
            }
          }   
        }
      }
    }
    // printf("*** %d %d\n",trip,pixelCount);
    if (_cds) {
      _result[currentDet]->addcontent(pixelCount);
      _result[currentDet]->valid(_clk);
    }
    if (trip) {
      float deltaT = _clk.asDouble() - _lastTrip.asDouble();
      if (deltaT>1.) {
        _lastTrip = _clk;
        printf("%d pixels over threshold in event %d, no trips in last %f seconds, attempt to trip, shunt, or ignore beam\n", pixelCount, _nevt, deltaT);
        if(_enableTrip[currentDet]) {
          if(_shutter) {
            *(dbr_enum_t*) _shutter->data() = 0;
            _shutter->put();
          } else {
            printf("No shutter pv configured, unable to close!\n");
          }
        } else {
          printf("Trip disabled for detector %d, not closing shutter!\n", currentDet);
        }
        ca_flush_io();
      }
    }

    _frame[currentDet]=0;
  }

  _nevt++;
}

//
//  Plug-in module creator
//

extern "C" UserModule* create() { return new JungfrauTripper; }

extern "C" void destroy(UserModule* p) { delete p; }
