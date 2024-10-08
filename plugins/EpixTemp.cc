#include "EpixTemp.hh"

#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"

#include "ami/app/NameService.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/psddl/alias.ddl.h"

#include <cstdio>

using namespace Ami;

EpixTempCache::EpixTempCache() :
  _config_size(0),
  _buffer(0)
{}

EpixTempCache::~EpixTempCache() {
  if (_buffer) delete[] _buffer;
}

void EpixTempCache::parse(void* payload) {
  const Pds::Epix::Config100aV2* orig_conf = reinterpret_cast<const Pds::Epix::Config100aV2*>(payload);
  if (orig_conf->_sizeof() > _config_size) {
    if (_buffer) delete[] _buffer;
    _buffer = new char[orig_conf->_sizeof()];
    memcpy(_buffer, orig_conf, orig_conf->_sizeof());
    _config_size = orig_conf->_sizeof();
  } else {
    memcpy(_buffer, orig_conf, orig_conf->_sizeof());
  }
}

const Pds::Epix::Config100aV2& EpixTempCache::config() {
  return *reinterpret_cast<const Pds::Epix::Config100aV2*>(_buffer);
}

const double EpixTemp::RDIV=20000;

EpixTemp::EpixTemp(const char* name, const char* short_name) :
  _cds(0),
  _epix_num(0),
  _fname(name),
  _sname(short_name),
  _name_service(new NameService)
{
  clear_data();
  // Initialize arrays
  for (uint det=0; det<NUM_EPIX; det++) {
    _epix_name[det] = "";
    _epix_pres[det] = false;
    _epix_config[det] = new EpixTempCache();
  }
}

EpixTemp::~EpixTemp() {
  if (_name_service) delete _name_service;
  for (uint det=0; det<NUM_EPIX; det++) {
    if (_epix_config[det]) delete _epix_config[det];
  }
}

const char* EpixTemp::name() const { return _fname; }

void EpixTemp::clock(const Pds::ClockTime& clk) { _clk=clk; }

void EpixTemp::clear_data() {
  for (uint det=0; det<NUM_EPIX; det++) {
    _epix_data[det] = 0;
  }
}

void EpixTemp::reset(FeatureCache&) {
  clear_data();
  // Clean up name service
  //_name_service->clear();
  // clean up data sources
  _epix_num = 0;
  for (uint det=0; det<NUM_EPIX; det++) {
    _epix_name[det] = "";
    _epix_pres[det] = false;
    remove(det);
  }
}

void EpixTemp::configure(const Pds::DetInfo&   src,
                         const Pds::TypeId&    type,
                         void*                 payload)
{
  // DAQ Epix detectors:
  if (type.id()==Pds::TypeId::Id_Epix100aConfig) {
    std::string id = std::string(Pds::DetInfo::name(src));
    printf("%s found DAQ Epix '%s'...",_sname,id.c_str());
    if (_epix_num < NUM_EPIX) {
      _epix_name[_epix_num] = id;
      _epix[_epix_num] = src;
      _epix_config[_epix_num]->parse(payload);
      _epix_pres[_epix_num] = true;
      _epix_data[_epix_num] = 0;
      if(_cds) create(*_cds, _epix_num);
      _epix_num++;
      printf("DID that work??");
    } else {
      printf("Found more DAQ Epix detectors than configured max of %d!\n", NUM_EPIX);
    }
    printf("\n");
  }
}

void EpixTemp::configure(const Pds::BldInfo&   src,
                         const Pds::TypeId&    type,
                         void*                 payload)
{
  // Nothing to do
}

void EpixTemp::configure(const Pds::ProcInfo&  src,
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
  }
}


//
//  Capture pointer to detector data we want for Pds::DetInfo objects
//
void EpixTemp::event(const Pds::DetInfo&   src,
                     const Pds::TypeId&    type,
                     const Pds::Damage&    dmg,
                     void*                 payload)
{
  // DAQ Epix detectrs
  if (type.id()==Pds::TypeId::Id_EpixElement) {
    if ((dmg.value() & DMG_MASK) == 0) {
      for (uint det=0; det<NUM_EPIX; det++) {
        if(_epix[det] == src and _epix_pres[det]) {
          _epix_data[det] = reinterpret_cast<const Pds::Epix::ElementV3*>(payload);
        }
      }
    }
  }
}

void EpixTemp::event(const Pds::BldInfo&   src,
                     const Pds::TypeId&    type,
                     const Pds::Damage&    dmg,
                     void*                 payload)
{
  // Nothing to do
}

void EpixTemp::event(const Pds::ProcInfo&  src,
                     const Pds::TypeId&    type,
                     const Pds::Damage&    dmg,
                     void*                 payload)
{
  // Nothing to do
}

//
//  Remove all plot entries
//
void EpixTemp::clear() 
{
  if (_cds) {
    for (uint det=0; det<NUM_EPIX; det++) {
      remove(det);
    }
    _cds = 0;
  }
}

//
//  Create all plot entries
//
void EpixTemp::create(Cds& cds)
{
  printf("EpixTemps::create plots\n");
  for (uint det=0; det<NUM_EPIX; det++) {
    if(_epix_pres[det]) create(cds, det);
  }

  _cds = &cds; 
}

void EpixTemp::create(Cds& cds, const uint epix_num) {
  char t[100];
  std::string base_titles[] = {"Temp 0#%s#0#0", "Temp 1#%s#0#1", "Humidity#%s#1#0"};
  std::string x_titles[] = {"temp [C]", "temp [C]", "humidity [%]"}; 
  std::string det_name = _epix_name[epix_num];
  const char* display_name = _name_service->name(_epix[epix_num]);
  if (!display_name) display_name = det_name.c_str();

  for(uint chan=0;chan<NUM_CHAN;chan++) {
    sprintf(t, base_titles[chan].c_str(), display_name);
    _epix_temp_plots[epix_num][chan] = new EntryScalar(DescScalar(t,x_titles[chan].c_str(),DescScalar::Mean,"",500));
    cds.add(_epix_temp_plots[epix_num][chan]);
  }

  _cds = &cds;
}

void EpixTemp::remove(const uint epix_num) {
  if (_cds) {
    for (uint chan=0; chan<NUM_CHAN; chan++) {
      if (_epix_temp_plots[epix_num][chan]) {
        _cds->remove(_epix_temp_plots[epix_num][chan]);
        _epix_temp_plots[epix_num][chan] = 0;
      }
    }
  }
}

//
//  Analyze data for current event from the pointers we captured
//
bool EpixTemp::accept () 
{
  if (_cds) {
    for (uint det=0; det<NUM_EPIX; det++) {
      if (_epix_data[det]) {
        ndarray<const uint32_t,2> env = _epix_data[det]->environmentalRows(_epix_config[det]->config());
        for (uint chan=0; chan<NUM_CHAN; chan++) {
          _epix_temp_plots[det][chan]->addcontent(env(env.shape()[0]-1,chan)*0.01);
          _epix_temp_plots[det][chan]->valid(_clk);
        }
      }
    }
  }

  //  Reset pointer references
  clear_data();

  return true;
}

//
//  Plug-in module creator
//

extern "C" UserModule* create() { return new EpixTemp; }

extern "C" void destroy(UserModule* p) { delete p; }
