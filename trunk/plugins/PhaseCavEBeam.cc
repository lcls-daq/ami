#include "PhaseCavEBeam.hh"

#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/DescScan.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/psddl/lusi.ddl.h"
#include "pdsdata/psddl/bld.ddl.h"

#include <string>
#include <iostream>

using namespace Ami;
using namespace std;

PhaseCavEBeam::PhaseCavEBeam() :
  XppBase("XPP-PHAS-EBEAM", "PhaseCavEBeam")
{}
PhaseCavEBeam::~PhaseCavEBeam() {}


//
//  Remove all plot entries
//
void PhaseCavEBeam::clear    () 
{
  if (_cds) {
    _cds->remove(_correbgas);
    _cds->remove(_ebeam_sb2);
    _cds->remove(_phasecav_t12);

    _cds = 0;
    _correbgas=0;
    _ebeam_sb2=0;
    _phasecav_t12=0;
  }
}

//
//  Create all plot entries
//
void PhaseCavEBeam::create   (Cds& cds)
{
  //title string:
  // plot_name (window) # tab_name # col_nr # row_nr # color

  //red, green, blue, black
  //  std::string colstr[4]={"#ff0000","#008000","#0000ff","#000000"};
  //as xpppython: black, red, green, blue, magenta, cyan, yellow
  std::string colstr[8]={"#000000","#ff0000","#008100","#0000ff","#bf00bf","#00bfbf","#ffff00","#999966"};
  std::string titlech[4];
  for (int i=0;i<4;i++){
    titlech[i] = "#" + std::string(1,'0'+char(i/2)) + "#"+std::string(1,'0'+char(i%2));
  }
  printf("PhaseCavEBeam::create plots\n");
  
  //now the phasecavity etc
  std::string titlePhas = "Phase Cav t1 vs t2#Phase Cav, EBeam #0#0";
  _phasecav_t12 = new EntryScan(DescScan(titlePhas.c_str(), "Phasecav t1","Phasecav t2", 200));
  cds.add(_phasecav_t12);
  std::string titleEbeamSB2 = "EBeam vs SB2 sum#Phase Cav, EBeam #0#1";
  _ebeam_sb2 = new EntryScan(DescScan(titleEbeamSB2.c_str(), "E Beam", "XPPSb2Ipm",200));
  cds.add(_ebeam_sb2);
  
  std::string titleEbgas = "Gas Det E vs E Beam #Phase Cav, EBeam #0#2"+colstr[0];
  _correbgas = new EntryScan(DescScan(titleEbgas.c_str(), "E Beam", "Gas Det E", 200));
  cds.add(_correbgas);


  _cds = &cds; 
}

//
//  Analyze data for current event from the pointers we captured
//
bool PhaseCavEBeam::accept () 
//bool PhaseCavEBeam::analyze  () 
{
//   printf("PhaseCavEBeam::accept _sb2_data(%p), _sb3_data(%p)  _sb2[%08x.%08x]\n",
//          _sb2_data, _sb3_data, _sb2.log(), _sb2.phy());

  if (_cds) {

    if (_phasecav_data) {
      _phasecav_t12->addy(_phasecav_data->fitTime2(), _phasecav_data->fitTime1());
      _phasecav_t12->valid(_clk);
    }

    if (_ebeam_data) {
      if (_ipm_data[IPM2]) {
	_ebeam_sb2->addy(_ipm_data[IPM2]->sum(),_ebeam_data->ebeamL3Energy());
	_ebeam_sb2->valid(_clk);
      }
      if (_feegas_data){
	_correbgas->addy(_feegas_data->f_11_ENRC(),_ebeam_data->ebeamL3Energy());
	_correbgas->valid(_clk);
      }
    }

  }
  //  Reset pointer references
  clear_data();

  return true;
}


extern "C" UserModule* create() { return new PhaseCavEBeam; }

extern "C" void destroy(UserModule* p) { delete p; }
