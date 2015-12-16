#include "CsPadTemp.hh"

#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"

#include "ami/app/NameService.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/cspad2x2.ddl.h"
#include "pdsdata/psddl/alias.ddl.h"

#include <cstdio>

using namespace Ami;

CsPadTemp::CsPadTemp(const char* name, const char* short_name) :
  _cds(0),
  _cspad_num(0),
  _cspad_2x2_num(0),
  _fname(name),
  _sname(short_name),
  _cspad_temp_conv(new CspadTemp(RDIV)),
  _name_service(new NameService)
{
  clear_data();
  // Initialize arrays
  for (uint det=0; det<NUM_CSPAD2x2; det++) {
    _cspad_2x2_name[det] = "";
    _cspad_2x2_pres[det] = false;
  }
  for (uint bigdet=0; bigdet<NUM_CSPAD; bigdet++) {
    _cspad_name[bigdet] = "";
    _cspad_pres[bigdet] = false;
  }
}

CsPadTemp::~CsPadTemp() {
  if (_cspad_temp_conv) delete _cspad_temp_conv;
  if (_name_service) delete _name_service;
}

const char* CsPadTemp::name() const { return _fname; }

void CsPadTemp::clock(const Pds::ClockTime& clk) { _clk=clk; }

void CsPadTemp::clear_data() {
  for (uint det=0; det<NUM_CSPAD2x2; det++) {
    _cspad_2x2_data[det] = 0;
  }
  for (uint bigdet=0; bigdet<NUM_CSPAD; bigdet++) {
    for (uint quad=0; quad<NUM_QUAD; quad++) {
      _cspad_data[bigdet][quad] = 0;
    }
  }
}

void CsPadTemp::reset(FeatureCache&) {
  clear_data();
  // Clean up name service
  //_name_service->clear();
  // clean up data sources
  _cspad_num = 0;
  _cspad_2x2_num = 0;
  for (uint det=0; det<NUM_CSPAD2x2; det++) {
    _cspad_2x2_name[det] = "";
    _cspad_2x2_pres[det] = false;
    remove(det);
  }
  for (uint bigdet=0; bigdet<NUM_CSPAD; bigdet++) {
    _cspad_name[bigdet] = "";
    _cspad_pres[bigdet] = false;
    remove(bigdet, true);
  }
}

void CsPadTemp::configure(const Pds::DetInfo&   src,
                          const Pds::TypeId&    type,
                          void*                 payload)
{
  // DAQ Cspads:
  if (type.id()==Pds::TypeId::Id_Cspad2x2Config) {
    std::string id = std::string(Pds::DetInfo::name(src));
    printf("%s found DAQ Cspad2x2 '%s'...",_sname,id.c_str());
    if (_cspad_2x2_num < NUM_CSPAD2x2) {
      _cspad_2x2_name[_cspad_2x2_num] = id;
      _cspad_2x2[_cspad_2x2_num] = src;
      _cspad_2x2_pres[_cspad_2x2_num] = true;
      _cspad_2x2_data[_cspad_2x2_num] = 0;
      if(_cds) create(*_cds, _cspad_2x2_num);
      _cspad_2x2_num++;
    } else {
      printf("Found more DAQ Cspad2x2s than configured max of %d!\n", NUM_CSPAD2x2);
    }
    printf("\n");
  }
  else if (type.id()==Pds::TypeId::Id_CspadConfig) {
    std::string id = std::string(Pds::DetInfo::name(src));
    printf("%s found DAQ Cspad '%s'...",_sname,id.c_str());
    if (_cspad_num < NUM_CSPAD) {
      _cspad_name[_cspad_num] = id;
      _cspad[_cspad_num] = src;
      _cspad_config[_cspad_num] = *reinterpret_cast<const Pds::CsPad::ConfigV5*>(payload);
      _cspad_pres[_cspad_num] = true;
      for (uint quad=0; quad<NUM_QUAD; quad++) {
        _cspad_data[_cspad_num][quad] = 0;
      }
      if(_cds) create(*_cds, _cspad_num, true);
      _cspad_num++;
    } else {
      printf("Found more DAQ Cspads than configured max of %d!\n", NUM_CSPAD);
    }
    printf("\n");
  }
}

void CsPadTemp::configure(const Pds::BldInfo&   src,
                          const Pds::TypeId&    type,
                          void*                 payload)
{
  // Nothing to do
}

void CsPadTemp::configure(const Pds::ProcInfo&  src,
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
void CsPadTemp::event(const Pds::DetInfo&   src,
                      const Pds::TypeId&    type,
                      const Pds::Damage&    dmg,
                      void*                 payload)
{
  // DAQ Cspads
  if (type.id()==Pds::TypeId::Id_Cspad2x2Element) {
    if ((dmg.value() & DMG_MASK) == 0) {
      for (uint det=0; det<NUM_CSPAD2x2; det++) {
        if(_cspad_2x2[det] == src and _cspad_2x2_pres[det]) {
          _cspad_2x2_data[det] = reinterpret_cast<const Pds::CsPad2x2::ElementV1*>(payload);
        }
      }
    }
  }
  else if (type.id()==Pds::TypeId::Id_CspadElement) {
    if ((dmg.value() & DMG_MASK) == 0) {
      for (uint bigdet=0; bigdet<NUM_CSPAD; bigdet++) {
        if(_cspad[bigdet] == src and _cspad_pres[bigdet]) {
          for (uint quad=0; quad<_cspad_config[bigdet].numQuads(); quad++) {
            _cspad_data[bigdet][quad] = &reinterpret_cast<const Pds::CsPad::DataV2*>(payload)->quads(_cspad_config[bigdet], quad);
          }
        }
      }
    }
  }
}

void CsPadTemp::event(const Pds::BldInfo&   src,
                      const Pds::TypeId&    type,
                      const Pds::Damage&    dmg,
                      void*                 payload)
{
  // Nothing to do
}

void CsPadTemp::event(const Pds::ProcInfo&  src,
                      const Pds::TypeId&    type,
                      const Pds::Damage&    dmg,
                      void*                 payload)
{
  // Nothing to do
}

//
//  Remove all plot entries
//
void CsPadTemp::clear() 
{
  if (_cds) {
    for (uint det=0; det<NUM_CSPAD2x2; det++) {
      remove(det);
    }
    for (uint bigdet=0; bigdet<NUM_CSPAD; bigdet++) {
      remove(bigdet, true);
    }
    _cds = 0;
  }
}

//
//  Create all plot entries
//
void CsPadTemp::create(Cds& cds)
{
  printf("CsPadTemps::create plots\n");
  for (uint det=0; det<NUM_CSPAD2x2; det++) {
    if(_cspad_2x2_pres[det]) create(cds, det);
  }
  for (uint bigdet=0; bigdet<NUM_CSPAD; bigdet++) {
    if(_cspad_pres[bigdet]) create(cds, bigdet, true);
  }

  _cds = &cds; 
}

void CsPadTemp::create(Cds& cds, const uint cspad_num, const bool is_big) {
  char t[100];
  if (is_big) {
    std::string  base_titles[] = {"Quad 0#%s#0#0", "Quad 1#%s#0#1", "Quad 2#%s#1#0", "Quad 3#%s#1#1"};
    std::string det_name = _cspad_name[cspad_num];
    const char* display_name = _name_service->name(_cspad[cspad_num]);
    if (!display_name) display_name = det_name.c_str();

    for(uint quad=0;quad<NUM_QUAD;quad++) {
      sprintf(t, base_titles[quad].c_str(), display_name);
      _cspad_temp_plots[cspad_num][quad] = new EntryScalar(DescScalar(t,"temp [C]",DescScalar::Mean,"",500));
      cds.add(_cspad_temp_plots[cspad_num][quad]);
    }
  } else {
    std::string  base_titles[] = {"Temp 0#%s#0#0", "Temp 1#%s#0#1", "Temp 2#%s#1#0", "Temp 3#%s#1#1"};
    std::string det_name = _cspad_2x2_name[cspad_num];
    const char* display_name = _name_service->name(_cspad_2x2[cspad_num]);
    if (!display_name) display_name = det_name.c_str();

    for(uint chan=0;chan<NUM_CHAN;chan++) {
      sprintf(t, base_titles[chan].c_str(), display_name);
      _cspad_2x2_temp_plots[cspad_num][chan] = new EntryScalar(DescScalar(t,"temp [C]",DescScalar::Mean,"",500));
      cds.add(_cspad_2x2_temp_plots[cspad_num][chan]);
    }
  }

  _cds = &cds;
}

void CsPadTemp::remove(const uint cspad_num, const bool is_big) {
  if (_cds) {
    if (is_big) {
      for (uint quad=0; quad<NUM_QUAD; quad++) {
        if (_cspad_temp_plots[cspad_num][quad]) {
          _cds->remove(_cspad_temp_plots[cspad_num][quad]);
          _cspad_temp_plots[cspad_num][quad] = 0;
        }
      }
    } else {
      for (uint chan=0; chan<NUM_CHAN; chan++) {
        if (_cspad_2x2_temp_plots[cspad_num][chan]) {
          _cds->remove(_cspad_2x2_temp_plots[cspad_num][chan]);
          _cspad_2x2_temp_plots[cspad_num][chan] = 0;
        }
      }
    }
  }
}

//
//  Analyze data for current event from the pointers we captured
//
bool CsPadTemp::accept () 
{
  if (_cds) {
    for (uint det=0; det<NUM_CSPAD2x2; det++) {
      if (_cspad_2x2_data[det]) {
        for (uint chan=0; chan<NUM_CHAN; chan++) {
          _cspad_2x2_temp_plots[det][chan]->addcontent(_cspad_temp_conv->getTemp(_cspad_2x2_data[det]->sb_temp()[chan]));
          _cspad_2x2_temp_plots[det][chan]->valid(_clk);
        }
      }
    }
    for (uint bigdet=0; bigdet<NUM_CSPAD; bigdet++) {
      if (_cspad_data[bigdet]) {
        for (uint quad=0; quad<NUM_QUAD; quad++) {
          if (_cspad_data[bigdet][quad]) {
            _cspad_temp_plots[bigdet][quad]->addcontent(_cspad_temp_conv->getTemp(_cspad_data[bigdet][quad]->sb_temp()[CSPAD_TEMP_CHAN]));
            _cspad_temp_plots[bigdet][quad]->valid(_clk);
          }
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

extern "C" UserModule* create() { return new CsPadTemp; }

extern "C" void destroy(UserModule* p) { delete p; }
