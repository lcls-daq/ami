#include "XPPIpm.hh"

#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/DescScan.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/DescTH1F.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/psddl/lusi.ddl.h"
#include "pdsdata/psddl/bld.ddl.h"

#include <string>
#include <iostream>

using namespace Ami;
using namespace std;

XPPIpm::XPPIpm() :
  XppBase("XPP-IPMs", "XppIpmSummary")
{}
XPPIpm::~XPPIpm() {}


//
//  Remove all plot entries
//
void XPPIpm::clear    () 
{
  if (_cds) {
    _cds->remove(_corr23);
    _cds->remove(_corr1_02);
    _cds->remove(_corr1_13);
    _cds->remove(_corr2_02);
    _cds->remove(_corr2_13);
    _cds->remove(_corr3_02);
    _cds->remove(_corr3_13);
    _cds->remove(_corr1_01);
    _cds->remove(_corr1_23);
    _cds->remove(_corr2_01);
    _cds->remove(_corr2_23);
    _cds->remove(_corr3_01);
    _cds->remove(_corr3_23);
    _cds->remove(_scan_sb2);
    _cds->remove(_scan_sb3);
    _cds->remove(_scan_gasdet);
    _cds->remove(_th1f_sb2);
    _cds->remove(_th1f_sb3);

    _cds = 0;
    _corr23=0;
    _corr1_02=0;
    _corr1_13=0;
    _corr2_02=0;
    _corr2_13=0;
    _corr3_02=0;
    _corr3_13=0;
    _corr1_01=0;
    _corr1_23=0;
    _corr2_01=0;
    _corr2_23=0;
    _corr3_01=0;
    _corr3_23=0;
    _scan_sb2=0;
    _scan_sb3=0;
    _scan_gasdet=0;
    _th1f_sb2=0;
    _th1f_sb3=0;

  }
}

//
//  Create all plot entries
//
void XPPIpm::create   (Cds& cds)
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
  printf("XPPIpm::create plots\n");
  
  std::string titleIPM23 = "SB2 vs SB3#Ipm Summary #0#0";
  _corr23 = new EntryScan(DescScan(titleIPM23.c_str(), "Ipm Sb2 Sum", "Ipm Sb3 Sum", 200));
  cds.add(_corr23); 
  
  std::string titleIPM2 = "Sum SB2 vs time#Ipm Summary #0#1"+colstr[0];
  std::string titleIPM3 = "Sum SB3 vs time#Ipm Summary #0#1"+colstr[1];
  std::string titleGasDet = "GasDet f11_ENRC vs time#Ipm Summary #0#1"+colstr[2];
  _scan_sb2 = new EntryScalar(DescScalar(titleIPM2.c_str(),"Ipm Sum SB2"));
  cds.add(_scan_sb2);
  _scan_sb3 = new EntryScalar(DescScalar(titleIPM3.c_str(),"Ipm Sum SB3"));
  cds.add(_scan_sb3);
  _scan_gasdet = new EntryScalar(DescScalar(titleGasDet.c_str(),"Gas Detector"));
  cds.add(_scan_gasdet);
  
  std::string titleIPM2b = "Sum SB2,3#Ipm Summary #0#2"+colstr[0];
  std::string titleIPM3b = "Sum SB3#Ipm Summary #0#2"+colstr[1];
  _th1f_sb2  = new EntryTH1F(DescTH1F(titleIPM2b.c_str(),"Ipm SB2 Sum","Entries",100,0.,10));
  _th1f_sb3  = new EntryTH1F(DescTH1F(titleIPM3b.c_str(),"Ipm SB3 Sum","Entries",100,0.,10));
  //_th1f_sb2  = new EntryTH1F(DescTH1F(titleIPM2b.c_str(),"Ipm SB2 Sum","Entries",1000,-0.01,0.1));
  //_th1f_sb3  = new EntryTH1F(DescTH1F(titleIPM3b.c_str(),"Ipm SB3 Sum","Entries",1000,-0.01,0.1));
  cds.add(_th1f_sb2);
  cds.add(_th1f_sb3);

  std::string titleIPM1_01 = "Ipm SB1, Channel 0/1#Ipm Channels #0#0"+colstr[0];
  std::string titleIPM1_23 = "Ipm SB1, Channel 2/3#Ipm Channels #0#0"+colstr[1];
  std::string titleIPM2_01 = "Ipm SB2, Channel 0/1#Ipm Channels #0#1"+colstr[0];
  std::string titleIPM2_23 = "Ipm SB2, Channel 2/3#Ipm Channels #0#1"+colstr[1];
  std::string titleIPM3_01 = "Ipm SB3, Channel 0/1#Ipm Channels #0#2"+colstr[0];
  std::string titleIPM3_23 = "Ipm SB3, Channel 2/3#Ipm Channels #0#2"+colstr[1];
  _corr1_01 = new EntryScan(DescScan(titleIPM1_01.c_str(), "Ipm SB1 Ch_A", "Ipm SB1 Ch_B", 200));
  _corr1_23 = new EntryScan(DescScan(titleIPM1_23.c_str(), "Ipm SB1 Ch_A", "Ipm SB1 Ch_B", 200));
  _corr2_01 = new EntryScan(DescScan(titleIPM2_01.c_str(), "Ipm SB2 Ch_A", "Ipm SB2 Ch_B", 200));
  _corr2_23 = new EntryScan(DescScan(titleIPM2_23.c_str(), "Ipm SB2 Ch_A", "Ipm SB2 Ch_B", 200));
  _corr3_01 = new EntryScan(DescScan(titleIPM3_01.c_str(), "Ipm SB3 Ch_A", "Ipm SB3 Ch_B", 200));
  _corr3_23 = new EntryScan(DescScan(titleIPM3_23.c_str(), "Ipm SB3 Ch_A", "Ipm SB3 Ch_B", 200));
  std::string titleIPMal1_02 = "Ipm SB1, Channel 0/2#Ipm Alignment #0#0"+colstr[0];
  std::string titleIPMal1_13 = "Ipm SB1, Channel 1/3#Ipm Alignment #0#0"+colstr[1];
  std::string titleIPMal2_02 = "Ipm SB2, Channel 0/2#Ipm Alignment #0#1"+colstr[0];
  std::string titleIPMal2_13 = "Ipm SB2, Channel 1/3#Ipm Alignment #0#1"+colstr[1];
  std::string titleIPMal3_02 = "Ipm SB3, Channel 0/2#Ipm Alignment #0#2"+colstr[0];
  std::string titleIPMal3_13 = "Ipm SB3, Channel 1/3#Ipm Alignment #0#2"+colstr[1];
  _corr1_02 = new EntryScan(DescScan(titleIPMal1_02.c_str(), "Ipm SB1 Ch_A", "Ipm SB1 Ch_B", 200));
  _corr1_13 = new EntryScan(DescScan(titleIPMal1_13.c_str(), "Ipm SB1 Ch_A", "Ipm SB1 Ch_B", 200));
  _corr2_02 = new EntryScan(DescScan(titleIPMal2_02.c_str(), "Ipm SB2 Ch_A", "Ipm SB2 Ch_B", 200));
  _corr2_13 = new EntryScan(DescScan(titleIPMal2_13.c_str(), "Ipm SB2 Ch_A", "Ipm SB2 Ch_B", 200));
  _corr3_02 = new EntryScan(DescScan(titleIPMal3_02.c_str(), "Ipm SB3 Ch_A", "Ipm SB3 Ch_B", 200));
  _corr3_13 = new EntryScan(DescScan(titleIPMal3_13.c_str(), "Ipm SB3 Ch_A", "Ipm SB3 Ch_B", 200));
  cds.add(_corr1_02);
  cds.add(_corr1_13);
  cds.add(_corr2_02);
  cds.add(_corr2_13);
  cds.add(_corr3_02);
  cds.add(_corr3_13);
  cds.add(_corr1_01);
  cds.add(_corr1_23);
  cds.add(_corr2_01);
  cds.add(_corr2_23);
  cds.add(_corr3_01);
  cds.add(_corr3_23);
    
  _cds = &cds; 
}

//
//  Analyze data for current event from the pointers we captured
//
bool XPPIpm::accept () 
//bool XPPIpm::analyze  () 
{
//   printf("XPPIpm::accept _sb2_data(%p), _sb3_data(%p)  _sb2[%08x.%08x]\n",
//          _sb2_data, _sb3_data, _sb2.log(), _sb2.phy());

  if (_cds) {
    if (_ipm_data[IPM2]){
      _scan_sb2->addcontent(_ipm_data[IPM2]->sum());
      _corr2_02->addy(_ipm_data[IPM2]->channel()[2] ,_ipm_data[IPM2]->channel()[0]);
      _corr2_13->addy(_ipm_data[IPM2]->channel()[3] ,_ipm_data[IPM2]->channel()[1]);
      _corr2_01->addy(_ipm_data[IPM2]->channel()[1] ,_ipm_data[IPM2]->channel()[0]);
      _corr2_23->addy(_ipm_data[IPM2]->channel()[3] ,_ipm_data[IPM2]->channel()[2]);
      _th1f_sb2->addcontent(1,_ipm_data[IPM2]->sum());
      _scan_sb2->valid(_clk);
      _corr2_02->valid(_clk);
      _corr2_13->valid(_clk);
      _corr2_01->valid(_clk);
      _corr2_23->valid(_clk);
      _th1f_sb2->valid(_clk);
    }
    if (_ipm_data[IPM3]){
      _scan_sb3->addcontent(_ipm_data[IPM3]->sum());
      _corr3_02->addy(_ipm_data[IPM3]->channel()[2] ,_ipm_data[IPM3]->channel()[0]);
      _corr3_13->addy(_ipm_data[IPM3]->channel()[3] ,_ipm_data[IPM3]->channel()[1]);
      _corr3_01->addy(_ipm_data[IPM3]->channel()[1] ,_ipm_data[IPM3]->channel()[0]);
      _corr3_23->addy(_ipm_data[IPM3]->channel()[3] ,_ipm_data[IPM3]->channel()[2]);
      _th1f_sb3->addcontent(1,_ipm_data[IPM3]->sum());
      _scan_sb3->valid(_clk);
      _corr3_02->valid(_clk);
      _corr3_13->valid(_clk);
      _corr3_01->valid(_clk);
      _corr3_23->valid(_clk);
      _th1f_sb3->valid(_clk);
    }

    if (_ipm_data[IPM2] && _ipm_data[IPM3]){
      _corr23->addy(_ipm_data[IPM3]->sum() ,_ipm_data[IPM2] ->sum() );
      _corr23->valid(_clk);

    }
    if (_ipm_data[IPM1]){
      _corr1_02->addy(_ipm_data[IPM1]->channel()[2] ,_ipm_data[IPM1]->channel()[0]);
      _corr1_13->addy(_ipm_data[IPM1]->channel()[3] ,_ipm_data[IPM1]->channel()[1]);
      _corr1_01->addy(_ipm_data[IPM1]->channel()[1] ,_ipm_data[IPM1]->channel()[0]);
      _corr1_23->addy(_ipm_data[IPM1]->channel()[3] ,_ipm_data[IPM1]->channel()[2]);
    }
    if (_feegas_data){
      _scan_gasdet->addcontent(_feegas_data->f_11_ENRC());
      _scan_gasdet->valid(_clk);
    }

  }
  //  Reset pointer references
  clear_data();
 return true;
}

//  Plug-in module creator
//

extern "C" UserModule* create() { return new XPPIpm; }

extern "C" void destroy(UserModule* p) { delete p; }
