#include "IpmSumEScan.hh"

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

IpmSumEScan::IpmSumEScan() :
  XppBase("XPP-IPM-EScan", "IpmSumEScan")
{}
IpmSumEScan::~IpmSumEScan() {}


//
//  Remove all plot entries
//
void IpmSumEScan::clear    () 
{
  if (_cds) {
    _cds->remove(_L3E_IPM2);
    _cds->remove(_L3E_IPM3);
    _cds->remove(_L3E_IPM0CH1);
    _cds->remove(_L3E_IPM0CH2);
    _cds->remove(_L3E_IPM0CH3);

    _cds = 0;
    _L3E_IPM2=0;
    _L3E_IPM3=0;
    _L3E_IPM0CH1=0;
    _L3E_IPM0CH2=0;
    _L3E_IPM0CH3=0;
  }
}

//
//  Create all plot entries
//
void IpmSumEScan::create   (Cds& cds)
{
  printf("IpmSumEScan::create plots\n");
  
  // Set up plot titles, axes, positions, etc.
  std::string titleSB2 = "L3E vs Ipm2 Sum#Ipm2 #0#0";
  _L3E_IPM2 = new EntryScan(DescScan(titleSB2.c_str(), "L3E","IPM2 SUM",300));
  cds.add(_L3E_IPM2);

  std::string titleSB3 = "L3E vs Ipm3 Sum#Ipm3 #1#0";
  _L3E_IPM3 = new EntryScan(DescScan(titleSB3.c_str(), "L3E","IPM3 SUM",300));
  cds.add(_L3E_IPM3);

  std::string titleIPM0CH0 = "L3E vs Ipm0 Ch0#Ipm0-Ch0 #2#0";
  _L3E_IPM0CH0 = new EntryScan(DescScan(titleIPM0CH0.c_str(), "L3E","IPM0 CH0",300));
  cds.add(_L3E_IPM0CH0);

  std::string titleIPM0CH1 = "L3E vs Ipm0 Ch1#Ipm0-Ch1 #3#0";
  _L3E_IPM0CH1 = new EntryScan(DescScan(titleIPM0CH1.c_str(), "L3E","IPM0 CH1",300));
  cds.add(_L3E_IPM0CH1);

  std::string titleIPM0CH2 = "L3E vs Ipm0 Ch2#Ipm0-Ch2 #4#0";
  _L3E_IPM0CH2 = new EntryScan(DescScan(titleIPM0CH2.c_str(), "L3E","IPM0 CH2",300));
  cds.add(_L3E_IPM0CH2);

  std::string titleIPM0CH3 = "L3E vs Ipm0 Ch3#Ipm0-Ch3 #5#0";
  _L3E_IPM0CH3 = new EntryScan(DescScan(titleIPM0CH3.c_str(), "L3E","IPM0 CH3",300));
  cds.add(_L3E_IPM0CH3);

  _cds = &cds; 
}

//
//  Analyze data for current event from the pointers we captured
//
bool IpmSumEScan::accept () 
{
  if (_cds) {
    if (_ebeam_data) {
      if (_ipm_data[IPM2]) {
	_L3E_IPM2->addy(_ipm_data[IPM2]->sum(),_ebeam_data->ebeamL3Energy());
	_L3E_IPM2->valid(_clk);
      }
      if (_ipm_data[IPM3]) {
        _L3E_IPM3->addy(_ipm_data[IPM3]->sum(),_ebeam_data->ebeamL3Energy());
        _L3E_IPM3->valid(_clk);
      }
      if (_ipm_data[IPMUSER]) {
        _L3E_IPM0CH0->addy(_ipm_data[IPMUSER]->channel()[0],_ebeam_data->ebeamL3Energy());
        _L3E_IPM0CH0->valid(_clk);
        _L3E_IPM0CH1->addy(_ipm_data[IPMUSER]->channel()[1],_ebeam_data->ebeamL3Energy());
        _L3E_IPM0CH1->valid(_clk);
        _L3E_IPM0CH2->addy(_ipm_data[IPMUSER]->channel()[2],_ebeam_data->ebeamL3Energy());
        _L3E_IPM0CH2->valid(_clk);
        _L3E_IPM0CH3->addy(_ipm_data[IPMUSER]->channel()[3],_ebeam_data->ebeamL3Energy());
        _L3E_IPM0CH3->valid(_clk);
      }
    }
  }
  //  Reset pointer references
  clear_data();

  return true;
}


extern "C" UserModule* create() { return new IpmSumEScan; }

extern "C" void destroy(UserModule* p) { delete p; }
