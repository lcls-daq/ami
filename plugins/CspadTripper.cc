#include "CspadTripper.hh"

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

using namespace Ami;

namespace Ami {

class Point {
public:
  Point(unsigned r, unsigned c) {
    _val = (r<<16)|c;
  }
  unsigned r() {return _val>>16 & 0xffff;}
  unsigned c() {return _val & 0xffff;}
private:
  unsigned _val;
};

class BlackHole {
public:
  typedef int32_t conmap_t;
  typedef int16_t data_t;
  BlackHole(unsigned nrows,  unsigned ncols, data_t thresh,
            unsigned holeNpixMin, unsigned holeNpixMax) :
    _thresh(thresh), _holeNpixMin(holeNpixMin), _holeNpixMax(holeNpixMax)
  {
    // the "connected map"
    _conmap = make_ndarray<conmap_t>(nrows, ncols);
    _stack.reserve(185*388*64);
    _ndump = 0;
  }
  bool floodFill(ndarray<const data_t, 3> data, uint quad, unsigned currentDet);
  const ndarray<conmap_t, 2>& conmap() {return _conmap;}
  unsigned numreg() {return _numreg;}
  unsigned lasttrip_sec() {return _lasttrip_sec;}
  unsigned lasttrip_pixcount() {return _lasttrip_pixcount;}
  void setupGoodPix(ndarray<const data_t, 3> data, uint quad, unsigned currentDet);
  void updateGoodPix(ndarray<const data_t, 3> data, uint quad, unsigned currentDet);

private:
  static const int MAXREGION = 50;
  static const unsigned MAXDUMP = 5;
  static const unsigned MAXPRINT = 50;
  unsigned _count[MAXREGION];
  bool _edge[MAXREGION];

  bool _maybePush(unsigned sec, unsigned r, unsigned c,
                  const ndarray<const data_t, 3>& data);
  bool _trip();
  void _dump(ndarray<const data_t, 3> data);
  ndarray<conmap_t, 2> _conmap;
  unsigned _numreg;
  data_t _thresh;
  unsigned _holeNpixMin;
  unsigned _holeNpixMax;
  unsigned _lasttrip_sec;
  unsigned _lasttrip_pixcount;
  unsigned _ndump,_nprint;
  std::vector<Point> _stack;
  ndarray<data_t, 4> _goodpix;
};

void BlackHole::_dump(ndarray<const data_t, 3> data) {
  if (_ndump>MAXDUMP) return;
  std::ofstream file;
  char fname[80];
  // unfortunately the event time and fiducials are
  // not available in AMI plugins
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  sprintf(fname,"/tmp/cspadtripper_%ld_%ld_%d.dat",now.tv_sec,
          now.tv_nsec,_ndump);
  file.open (fname, std::ios::out | std::ios::binary);
  if (!file.is_open()) return;
  file.write((char*)data.data(),data.size()*sizeof(data_t));
  file.close();
  _ndump++;
}

bool BlackHole::_maybePush(unsigned seg, unsigned r, unsigned c,
                           const ndarray<const data_t, 3>& data) {
  // check for unassigned pixel
  if (_conmap(r,c)==-1) {
    if (data(seg,r,c)>_thresh) {
      _conmap(r,c)=0; // above threshold
    }
    else {
      _stack.push_back(Point(r,c)); // connected pixel
      return true;
    }
  }
  return false;
}

void BlackHole::setupGoodPix(ndarray<const data_t, 3> data, uint quad,
                             unsigned currentDet) {
  if (_goodpix.empty()) {
    printf("*** make array\n");
    _goodpix = make_ndarray<data_t>(MaxDetsAllowed*Pds::CsPad::MaxQuadsPerSensor,
                                    data.shape()[0],data.shape()[1],
                                    data.shape()[2]);
  }

  unsigned nbadpix=0;
  for(unsigned s = 0; s<data.shape()[0]; s++) {
    for(unsigned r = 0; r<data.shape()[1]; r++) {
      for(unsigned c = 0; c<data.shape()[2]; c++) {
        if (data(s,r,c)<(_thresh-1000)) {
          _goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c) = 1;
        } else {
          _goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c) = 0;
          nbadpix++;
        }
      }
    }
  }
  printf("--- Found %d initial badpix in quad %d\n",nbadpix,quad);
}

void BlackHole::updateGoodPix(ndarray<const data_t, 3> data, uint quad,
                              unsigned currentDet) {
  unsigned nbadpix=0;
  for(unsigned s = 0; s<data.shape()[0]; s++) {
    for(unsigned r = 0; r<data.shape()[1]; r++) {
      for(unsigned c = 0; c<data.shape()[2]; c++) {
        // check to see if the pixel is bad for two events in a row
        if (data(s,r,c)<(_thresh-1000) && !_goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c)) {
          _goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c) = 1;
        }
        if (!_goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,s,r,c)) nbadpix++;
      }
    }
  }
  printf("*** found %d bad pixels in quad %d\n",nbadpix,quad);
}

bool BlackHole::floodFill(ndarray<const data_t, 3> data, uint quad, unsigned currentDet) {
  for(unsigned sec = 0; sec<data.shape()[0]; sec++) {
    _numreg=0;
    std::fill_n(_conmap.data(), int(_conmap.size()), conmap_t(-1));
    unsigned nrows = data.shape()[1];
    unsigned ncols = data.shape()[2];
    unsigned nabovethresh = 0;
    for(unsigned row = 0; row<nrows; row++) {
      for(unsigned col = 0; col<ncols; col++) {
        if (data(sec,row,col)>_thresh && _goodpix(currentDet*Pds::CsPad::MaxQuadsPerSensor+quad,sec,row,col)) nabovethresh++;
      }
    }
    // don't bother with flood-fill if we don't have enough
    // pixels above threshold
    if (nabovethresh<_holeNpixMin+10) continue;
    printf("*** Found %d above thresh in quad %d section %d\n",nabovethresh, quad, sec);
    for(unsigned row = 0; row<nrows; row++) {
      for(unsigned col = 0; col<ncols; col++) {

        if (_maybePush(sec,row,col,data)) {
          ++ _numreg; // start a new region
        } else {
          continue;
        }
        while (!_stack.empty()) {
          Point val = _stack.back();
          _stack.pop_back();
          unsigned r = val.r();
          unsigned c = val.c();
          _conmap(r,c) = _numreg;

          if (r>0)       _maybePush(sec,r-1,c,data);
          if (r<nrows-1) _maybePush(sec,r+1,c,data);
          if (c>0)       _maybePush(sec,r,c-1,data);
          if (c<ncols-1) _maybePush(sec,r,c+1,data);
        }
      }
    }
    bool trip = _trip();
    if (trip) {
      _lasttrip_sec = sec;
      if (_nprint<MAXPRINT) {
        printf("*** Found blackhole with %d pixels at time %lu\n",
               lasttrip_pixcount(),(unsigned long)time(NULL));
        _nprint++;
      }
      _dump(data);
      return true;
    }
  }
  return false;
}

bool BlackHole::_trip() {
  std::fill_n(_count, MAXREGION, unsigned(0));
  std::fill_n(_edge, MAXREGION, bool(0));

  for(unsigned r = 0; r<_conmap.shape()[0]; r++) {
    for(unsigned c = 0; c<_conmap.shape()[1]; c++) {
      conmap_t region = _conmap(r,c);
      if (_conmap(r,c)==0) continue;
      if (region==0) continue; // ignore above-threshold pixels
      region -= 1; // to count from zero
      if (region >= MAXREGION) continue; // shouldn't happen
      _count[region]++;
      // ignore any region that has an edge in it (i.e. not
      // completely enclosed)
      if (!_edge[region]) {
        _edge[region] = (r==0) || (r==_conmap.shape()[0]-1) ||
          (c==0) || (c==_conmap.shape()[1]-1);
      }
    }
  }

  for (unsigned i=0; i<_numreg; i++) {
    // trip if we have correct-number of below-threshold pixels
    // not at an edge (we could imagine removing the edge requirement)
    if (_count[i]>=_holeNpixMin && _count[i]<=_holeNpixMax && !_edge[i]) {
      _lasttrip_pixcount = _count[i];
      return true;
    }
  }
  return false;
}
};

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
    _config_size[i]=0;
  }
  _shutter_pv = new char[64];

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
  _bh = new BlackHole(Pds::CsPad::ColumnsPerASIC,
                      Pds::CsPad::MaxRowsPerASIC*2,
                      12000,8,200);
}

CspadTripper::~CspadTripper()
{
  if (_bh)          delete    _bh;
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
    _config_size[id]=0;
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
  if (type.id()==Pds::TypeId::Id_CspadConfig) {

    _nevt   = 0;

    // need to copy the configuration, since it doesn't persist
    const Pds::CsPad::ConfigV5* orig_conf = reinterpret_cast<const Pds::CsPad::ConfigV5*>(payload);
    if (orig_conf->_sizeof() > _config_size[_detsFound]) {
      // need to allocate a larger buffer
      if (_config[_detsFound]) delete[] (char *)(_config[_detsFound]);
      char* buffer = new char[orig_conf->_sizeof()];
      _config[_detsFound] = reinterpret_cast<Pds::CsPad::ConfigV5*>(buffer);
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
  if (type.id()==Pds::TypeId::Id_CspadElement) {
    if ((dmg.value() & DMG_MASK) == 0) {
      for (unsigned id=0; id<_detsFound; id++) {
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
  // Note that the way _frame is set ensures that multiple cspads
  // always show up in the same order in the _frame array.  this
  // ensures we will use the correct bad pixel map with multiple
  // detectors. - cpo

  for(unsigned currentDet=0;currentDet<_detsFound;currentDet++) {
    if (!_frame[currentDet]) continue;

    bool trip = false;
    int32_t bh_pixelCount = 0;

    for (uint quad=0; quad<_config[currentDet]->numQuads(); quad++) {
      const Pds::CsPad::ElementV2& s = _frame[currentDet]->quads(*_config[currentDet], quad);
      const Pds::CsPad::ConfigV5& cfg = *_config[currentDet];

      ndarray<const int16_t, 3> data = s.data(cfg);
      if (_nevt==0) {
        _bh->setupGoodPix(data,s.quad(),currentDet);
      } else if (_nevt==1) {
        _bh->updateGoodPix(data,s.quad(),currentDet);
      } else {
        trip =_bh->floodFill(data,s.quad(),currentDet);
        if (trip) {
          bh_pixelCount = _bh->lasttrip_pixcount();
          break;
        }
      }
    }
    // printf("*** %d %d\n",trip,bh_pixelCount);
    if (_cds) {
      _result[currentDet]->addcontent(bh_pixelCount);
      _result[currentDet]->valid(_clk);
    }
    if (trip) {
      float deltaT = _clk.asDouble() - _lastTrip.asDouble();
      if (deltaT>1.) {
        _lastTrip = _clk;
        printf("%d pixels in blackhole at event %d, no trips in last %f seconds, attempt to trip, shunt, or ignore beam\n", bh_pixelCount, _nevt, deltaT);
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
