#include "CspadTripper.hh"

#include "ami/data/Cds.hh"

#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"

#include "ami/app/NameService.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/psddl/cspad2x2.ddl.h"
#include "pdsdata/psddl/alias.ddl.h"
#include "pdsdata/psddl/epics.ddl.h"

// its preprocesser does ugly things....
#include "ami/plugins/PVWriter.hh"

#include <fstream>
#include <sstream>
#include <string>

static const char* path = getenv("HOME");

using namespace Ami;

/*static unsigned countSaturatedPixels(const Pds::CsPad2x2::ElementV1& s, int32_t tripCountThreshold)
{
  unsigned saturatedPixels = 0;
  const unsigned COLS = Pds::CsPad2x2::ColumnsPerASIC;
  const unsigned ROWS = Pds::CsPad2x2::MaxRowsPerASIC*2;
  const unsigned SECS = Pds::CsPad2x2::SectorsPerQuad;
  for(unsigned col=0; col<COLS; col++)
    for(unsigned row=0; row<ROWS; row++) {
      for(unsigned sec=0; sec<SECS; sec++) {
        if(s.data()[col][row][sec] > tripCountThreshold) saturatedPixels++;
      }
    }
    return saturatedPixels;
}*/

static unsigned countSaturatedPixels(const Pds::CsPad::ElementV2& s, const Pds::CsPad::ConfigV5& cfg, int32_t tripCountThreshold)
{
  //  unsigned pixelCount = 0;
  unsigned saturatedPixels = 0;
  const unsigned COLS = Pds::CsPad::ColumnsPerASIC;
  const unsigned ROWS = Pds::CsPad::MaxRowsPerASIC;
  const unsigned SECS = Pds::CsPad::SectorsPerQuad;
  for(unsigned col=0; col<COLS; col++)
    for(unsigned row=0; row<ROWS; row++) {
      for(unsigned sec=0; sec<SECS; sec++) {
      if(s.data(cfg)[sec][col][row] > tripCountThreshold) saturatedPixels++;
      //if(s.data()[col][row+ROWS] > tripCountThreshold) saturatedPixels++;
    }
  }
  return saturatedPixels;
}

CspadTripper::CspadTripper(const char* name, const char* short_name) :
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
           "cspadtripper calling ca_context_create" );

  for (unsigned i=0;i<MaxDetsAllowed;i++) {
    _frame[i]=0;
    _config[i]=0;
    _dets[i]=0;
  }
  _shutter_pv = new char[64];
  //  printf("have called tripper constructor\n");
  //  sprintf(_nameBuffer, "CspadTripperTitleOfSomeSort");
  char fpath[128];
  sprintf(fpath,"CspadTripperPVs.lst");
  std::string line;
  std::ifstream file;
  file.open(fpath);
  if(!file.good()) {
    printf("tripper::configure unable to open %s\n",fpath);
    sprintf(fpath,"%s/CspadTripperPVs.lst",path);
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
        sprintf(_shutter_pv, "%s:CLOSE", line.c_str());
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

CspadTripper::~CspadTripper()
{
  if (_name_service) delete   _name_service;
  if (_shutter)     delete    _shutter;
  if (_thres_pv)    delete[]  _thres_pv;
  if (_npixel_pv)   delete[]  _npixel_pv;
  if (_enable_pv)   delete[]  _enable_pv;
  if (_shutter_pv)  delete[]  _shutter_pv;

  ca_context_destroy();
}

void CspadTripper::reset    (FeatureCache& f)
{
  for (unsigned id=0; id<_detsFound; id++) {
    _config[id]=0;
    _dets[id]=0;
    if(_cds) _cds->remove(_result[id]);
  }
  _detsFound=0;
}
void CspadTripper::clock    (const Pds::ClockTime& clk) { _clk=clk; }
void CspadTripper::configure(const Pds::ProcInfo&    src,
                              const Pds::TypeId&    type,
                              void*                 payload){}
void CspadTripper::configure(const Pds::BldInfo&    src,
                              const Pds::TypeId&    type,
                              void*                 payload){}
void CspadTripper::configure(const Pds::DetInfo&    src,
                              const Pds::TypeId&    type,
                              void*                 payload) 
{
  //  printf("tripper configure\n");
  if (type.id()==Pds::TypeId::Id_CspadConfig) {
    //  printf("*************************tripper configure found cspadconfigtype*****************************\n");

    _nevt   = 0;
    _config[_detsFound] = reinterpret_cast<const Pds::CsPad::ConfigV5*>(payload);
    _dets[_detsFound] = src.phy();
    //  printf("found config for det %d, id 0x%x\n", _detsFound,  _dets[_detsFound]);

    _nPixelsToTrip[_detsFound] = 0x100000;
    _tripCountThreshold[_detsFound] = 0x4000;
    _enableTrip[_detsFound] = false;
    printf("have configured tripper with nPixelsToTrip %d, tripCountThreshold %d\n", _nPixelsToTrip[_detsFound], _tripCountThreshold[_detsFound]);
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

const char* CspadTripper::name() const {
  return _fname;
}


void CspadTripper::event    (const Pds::BldInfo&    src,
                             const Pds::TypeId&    type,
                             const Pds::Damage&     dmg,
                             void*                 payload)
{}

void CspadTripper::event    (const Pds::ProcInfo&    src,
                             const Pds::TypeId&    type,
                             const Pds::Damage&     dmg,
                             void*                 payload)
{}

//
//  Capture pointer to detector data we want
//
void CspadTripper::event    (const Pds::DetInfo&    src,
                             const Pds::TypeId&    type,
                             const Pds::Damage&     dmg,
                             void*                 payload) 
{
  //  printf("tripper event, payload size 0x%x\n", sizeof(payload));
  //  printf("need this print statement for some reason\n");
  if (type.id()==Pds::TypeId::Id_CspadElement) {
    if ((dmg.value() & DMG_MASK) == 0) {
      for (unsigned id=0; id<_detsFound; id++) {
        //      printf("looking for 0x%x, check 0x%x at %d\n", src.phy(), _dets[id], id);
        if (src.phy()==_dets[id]) _frame[id] = reinterpret_cast<const Pds::CsPad::DataV2*>(payload);
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

void CspadTripper::clear    () 
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

void CspadTripper::create   (Cds& cds)
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
bool CspadTripper::accept  () 
{
  analyzeDetector();
  return true;
}

void CspadTripper::analyzeDetector  () 
{
  //  printf("in analyze, look for frame\n");
  for(unsigned currentDet=0;currentDet<_detsFound;currentDet++) {
    if (!_frame[currentDet]) continue;

    int32_t pixelCount = 0;
    for (uint quad=0; quad<_config[currentDet]->numQuads(); quad++) {
      pixelCount += countSaturatedPixels(_frame[currentDet]->quads(*_config[currentDet], quad), *_config[currentDet], _tripCountThreshold[currentDet]);
    }
    if (_cds) {
      _result[currentDet]->addcontent(pixelCount);
      _result[currentDet]->valid(_clk);
    }
    if (pixelCount>=_nPixelsToTrip[currentDet]) {
      float deltaT = _clk.seconds() - _lastTrip.seconds() + (_clk.nanoseconds()-_lastTrip.nanoseconds())/1000000000.;
      if (deltaT>1.) {
        _lastTrip = _clk;
        printf("Have counted %d pixels (trip at %d) over %d at event %d, no trips in last %f seconds, attempt to trip, shunt, or ignore beam\n", pixelCount, _nPixelsToTrip[currentDet], _tripCountThreshold[currentDet], _nevt, deltaT);
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

extern "C" UserModule* create() { return new CspadTripper; }

extern "C" void destroy(UserModule* p) { delete p; }
