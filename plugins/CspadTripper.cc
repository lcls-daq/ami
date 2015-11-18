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

using namespace Ami;

static const char* path = getenv("HOME");


static unsigned countSaturatedPixels(const Pds::CsPad2x2::ElementV1& s, unsigned tripCountThreshold)
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
}

static unsigned countSaturatedPixels(const Pds::CsPad::ElementV2& s, const Pds::CsPad::ConfigV5& cfg, unsigned tripCountThreshold)
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
  _fname(name),
  _sname(short_name),
  _name_service(new NameService)
{
  _frame = 0;
  _frame_140k = 0;
  //  printf("have called tripper constructor\n");
  //  sprintf(_nameBuffer, "CspadTripperTitleOfSomeSort");
}
CspadTripper::~CspadTripper() {}

void CspadTripper::reset    (FeatureCache& f) {_detsFound=0;}
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
  if (type.id()!=Pds::TypeId::Id_CspadConfig) return;
  //  printf("*************************tripper configure found cspadconfigtype*****************************\n");

  _nevt   = 0;
  _config[_detsFound] = reinterpret_cast<const Pds::CsPad::ConfigV5*>(payload);
  _dets[_detsFound] = src.phy();
  //  printf("found config for det %d, id 0x%x\n", _detsFound,  _dets[_detsFound]);

  _nPixelsToTrip[_detsFound] = 0x100000;
  _tripCountThreshold[_detsFound] = 0x4000;
  char fpath[128];
  sprintf(fpath,"CspadTripper.%08x.lst", _dets[_detsFound]);
  FILE* f = fopen(fpath,"r");
  if (!f) {
    printf("tripper::configure unable to open %s\n",fpath);
    char fpath[128];
    sprintf(fpath,"%s/CspadTripper.%08x.lst",path,_dets[_detsFound]);
    f = fopen(fpath,"r");
    if (!f) {
      printf("tripper::configure unable to open %s, use default values\n",fpath);
      _detsFound++;
      return;
    }
  }
  //  printf("tripper::configure opened %s\n",fpath);
  char line[1024];
  char* lptr = &line[0];
  size_t line_sz = 1024;
  while(getline(&lptr,&line_sz,f)!=-1) {
    if (line[0]!='#') {
      _tripCountThreshold[_detsFound] = strtoul(lptr,&lptr,0);
      _nPixelsToTrip[_detsFound] = strtoul(lptr,&lptr,0);
      break;
    }
  }
  printf("have configured tripper with nPixelsToTrip %d, tripCountThreshold %d\n", _nPixelsToTrip[_detsFound], _tripCountThreshold[_detsFound]);
  _detsFound++;
}

const char* CspadTripper::name() const {
  return "CspadTripperModule";//_nameBuffer;
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
  _currentDet = -1;
  if (type.id()==Pds::TypeId::Id_CspadElement) {
    if ((dmg.value() & DMG_MASK) == 0) {
      _frame = reinterpret_cast<const Pds::CsPad::DataV2*>(payload);
      for (unsigned id=0; id<_detsFound; id++) {
        //      printf("looking for 0x%x, check 0x%x at %d\n", src.phy(), _dets[id], id);
        if (src.phy()==_dets[id]) _currentDet = id;
      }
      //    printf("have decided on 0x%x = 0x%x at %d\n", src.phy(), _dets[_currentDet], _currentDet);
      analyzeDetector();
    }
  }
  else if (type.id()==Pds::TypeId::Id_Cspad2x2Element) {
    if ((dmg.value() & DMG_MASK) == 0) {
      //    printf("probably out of date, skipping for now\n");
      _frame_140k = reinterpret_cast<const Pds::CsPad2x2::ElementV1*>(payload);
      for (unsigned id=0; id<_detsFound; id++) {
        //      printf("looking for 0x%x, check 0x%x at %d\n", src.phy(), _dets[id], id);
        if (src.phy()==_dets[id]) _currentDet = id;
      }
    }
    //    printf("have decided on 0x%x = 0x%x at %d\n", src.phy(), _dets[_currentDet], _currentDet);
    //    printf("but ignore for now\n");
    //    analyzeDetector();
    //  } else {
    //    printf("did not recognize type %d, trouble\n", type.id());
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
  //  printf("have called fake accept, don't want to do anything here\n");
  return true;
}

void CspadTripper::analyzeDetector  () 
{
  //  printf("in analyze, look for cds\n");
  if (!_cds) return;
  //  printf("in analyze, look for frame\n");
  if (!_frame and !_frame_140k) return;
  //  printf("in analyze, found frame\n");

  unsigned pixelCount = 0;
  if (_frame_140k) {
    pixelCount += countSaturatedPixels(*_frame_140k, _tripCountThreshold[_currentDet]);
  } else {
    for (uint quad=0; quad<_config[_currentDet]->numQuads(); quad++) {
      pixelCount += countSaturatedPixels(_frame->quads(*_config[_currentDet], quad), *_config[_currentDet], _tripCountThreshold[_currentDet]);
    }    
  }
  //  printf("pixel count is 0x%x for 0x%x (det %d)\n", pixelCount, _dets[_currentDet], _currentDet);
  _result[_currentDet]->addcontent(pixelCount);
  _result[_currentDet]->valid(_clk);
  //  printf("Have counted %d pixels at event %d, decide whether to attempt to trip, shunt, or ignore beam",pixelCount, _nevt);

  if (pixelCount>=_nPixelsToTrip[_currentDet]) {
    float deltaT = _clk.seconds() - _lastTrip.seconds() + (_clk.nanoseconds()-_lastTrip.nanoseconds())/1000000000.;
    if (deltaT>1.) {
      _lastTrip = _clk;
      printf("Have counted %d pixels (trip at %d) over %d at event %d, no trips in last %f seconds, attempt to trip, shunt, or ignore beam\n", pixelCount, _nPixelsToTrip[_currentDet], _tripCountThreshold[_currentDet], _nevt, deltaT);
      //system("caput myPV");
      system("/bin/env PATH=/reg/g/pcds/package/epics/3.14/base/current/bin/linux-x86_64 caput -c 'CXI:R48:EVR:41:CTRL.DG0P' 0");
    }
  }

  _nevt++;
  
  //  Reset pointer references
  _frame = 0;
}

//
//  Plug-in module creator
//

extern "C" UserModule* create() { return new CspadTripper; }

extern "C" void destroy(UserModule* p) { delete p; }
