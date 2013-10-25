#include "XPPIpmSB1.hh"

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

XPPIpmSB1::XPPIpmSB1() : 
  XppBase("XPP-IPM-SB1s", "XppIpmSb1Summary")
 {}
XPPIpmSB1::~XPPIpmSB1() {}

//
//  Remove all plot entries
//
void XPPIpmSB1::clear    () 
{
  if (_cds) {
    _cds->remove(_corr1_02);
    _cds->remove(_corr1_13);
    _cds->remove(_corr1c_02);
    _cds->remove(_corr1c_13);
    _cds->remove(_corr1_01);
    _cds->remove(_corr1_23);
    _cds->remove(_corr1c_01);
    _cds->remove(_corr1c_23);

    _cds->remove(_corrgdet1_0);
    _cds->remove(_corrgdet1_1);
    _cds->remove(_corrgdet1_2);
    _cds->remove(_corrgdet1_3);
    _cds->remove(_corrgdet1c_0);
    _cds->remove(_corrgdet1c_1);
    _cds->remove(_corrgdet1c_2);
    _cds->remove(_corrgdet1c_3);

    _cds->remove(_corr21_0);
    _cds->remove(_corr21_1);
    _cds->remove(_corr21_2);
    _cds->remove(_corr21_3);
    _cds->remove(_corr21c_0);
    _cds->remove(_corr21c_1);
    _cds->remove(_corr21c_2);
    _cds->remove(_corr21c_3);

    _cds->remove(_corruser1_0);
    _cds->remove(_corruser1_1);
    _cds->remove(_corruser1_2);
    _cds->remove(_corruser1_3);
    _cds->remove(_corruser1c_0);
    _cds->remove(_corruser1c_1);
    _cds->remove(_corruser1c_2);
    _cds->remove(_corruser1c_3);

    _cds = 0;
    _corr1_02=0;
    _corr1_13=0;
    _corr1c_02=0;
    _corr1c_13=0;
    _corr1_01=0;
    _corr1_23=0;
    _corr1c_01=0;
    _corr1c_23=0;
    _corrgdet1_0=0;
    _corrgdet1_1=0;
    _corrgdet1_2=0;
    _corrgdet1_3=0;
    _corrgdet1c_0=0;
    _corrgdet1c_1=0;
    _corrgdet1c_2=0;
    _corrgdet1c_3=0;
    _corr21_0=0;
    _corr21_1=0;
    _corr21_2=0;
    _corr21_3=0;
    _corr21c_0=0;
    _corr21c_1=0;
    _corr21c_2=0;
    _corr21c_3=0;
    _corruser1_0=0;
    _corruser1_1=0;
    _corruser1_2=0;
    _corruser1_3=0;
    _corruser1c_0=0;
    _corruser1c_1=0;
    _corruser1c_2=0;
    _corruser1c_3=0;
  }
}

//
//  Create all plot entries
//
void XPPIpmSB1::create   (Cds& cds)
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
  printf("XPPIpmSB1::create plots\n");
  
  std::string titleIPM1_01 = "Ipm SB1, Channel 0/1#Ipm Channels #0#0"+colstr[0];
  std::string titleIPM1_23 = "Ipm SB1, Channel 2/3#Ipm Channels #0#0"+colstr[1];
  std::string titleIPM1c_01 = "Ipm SB1c, Channel 0/1#Ipm Channels #0#1"+colstr[0];
  std::string titleIPM1c_23 = "Ipm SB1c, Channel 2/3#Ipm Channels #0#1"+colstr[1];
  _corr1_01 = new EntryScan(DescScan(titleIPM1_01.c_str(), "Ipm SB1 Ch_A", "Ipm SB1 Ch_B", 200));
  _corr1_23 = new EntryScan(DescScan(titleIPM1_23.c_str(), "Ipm SB1 Ch_A", "Ipm SB1 Ch_B", 200));
  _corr1c_01 = new EntryScan(DescScan(titleIPM1c_01.c_str(), "Ipm SB1c Ch_A", "Ipm SB1c Ch_B", 200));
  _corr1c_23 = new EntryScan(DescScan(titleIPM1c_23.c_str(), "Ipm SB1c Ch_A", "Ipm SB1c Ch_B", 200));

  std::string titleIPM1g_0 = "Ipm SB1 0, GasDet#Ipm Channels #1#0"+colstr[0];
  std::string titleIPM1g_1 = "Ipm SB1 1, GasDet#Ipm Channels #1#0"+colstr[1];
  std::string titleIPM1g_2 = "Ipm SB1 2, GasDet#Ipm Channels #1#0"+colstr[2];
  std::string titleIPM1g_3 = "Ipm SB1 3, GasDet#Ipm Channels #1#0"+colstr[3];
  std::string titleIPM1cg_0 = "Ipm SB1c 0, GasDet#Ipm Channels #1#1"+colstr[0];
  std::string titleIPM1cg_1 = "Ipm SB1c 1, GasDet#Ipm Channels #1#1"+colstr[1];
  std::string titleIPM1cg_2 = "Ipm SB1c 2, GasDet#Ipm Channels #1#1"+colstr[2];
  std::string titleIPM1cg_3 = "Ipm SB1c 3, GasDet#Ipm Channels #1#1"+colstr[3];
  _corrgdet1_0 = new EntryScan(DescScan(titleIPM1g_0.c_str(), "Ipm SB1 Ch_0", "Gas Det", 200));
  _corrgdet1_1 = new EntryScan(DescScan(titleIPM1g_1.c_str(), "Ipm SB1 Ch_1", "Gas Det", 200));
  _corrgdet1_2 = new EntryScan(DescScan(titleIPM1g_2.c_str(), "Ipm SB1 Ch_2", "Gas Det", 200));
  _corrgdet1_3 = new EntryScan(DescScan(titleIPM1g_3.c_str(), "Ipm SB1 Ch_3", "Gas Det", 200));
  _corrgdet1c_0 = new EntryScan(DescScan(titleIPM1cg_0.c_str(), "Ipm SB1 Ch_0", "Gas Det", 200));
  _corrgdet1c_1 = new EntryScan(DescScan(titleIPM1cg_1.c_str(), "Ipm SB1 Ch_1", "Gas Det", 200));
  _corrgdet1c_2 = new EntryScan(DescScan(titleIPM1cg_2.c_str(), "Ipm SB1 Ch_2", "Gas Det", 200));
  _corrgdet1c_3 = new EntryScan(DescScan(titleIPM1cg_3.c_str(), "Ipm SB1 Ch_3", "Gas Det", 200));
  cds.add(_corrgdet1_0);
  cds.add(_corrgdet1_1);
  cds.add(_corrgdet1_2);
  cds.add(_corrgdet1_3);
  cds.add(_corrgdet1c_0);
  cds.add(_corrgdet1c_1);
  cds.add(_corrgdet1c_2);
  cds.add(_corrgdet1c_3);

  std::string titleIPM12_0 = "Ipm SB1 0, SB2 Sum#Ipm Correlations #0#0"+colstr[0];
  std::string titleIPM12_1 = "Ipm SB1 1, SB2 Sum#Ipm Correlations #0#0"+colstr[1];
  std::string titleIPM12_2 = "Ipm SB1 2, SB2 Sum#Ipm Correlations #0#0"+colstr[2];
  std::string titleIPM12_3 = "Ipm SB1 3, SB2 Sum#Ipm Correlations #0#0"+colstr[3];
  std::string titleIPM1c2_0 = "Ipm SB1c 0, SB2 Sum#Ipm Correlations #0#1"+colstr[0];
  std::string titleIPM1c2_1 = "Ipm SB1c 1, SB2 Sum#Ipm Correlations #0#1"+colstr[1];
  std::string titleIPM1c2_2 = "Ipm SB1c 2, SB2 Sum#Ipm Correlations #0#1"+colstr[2];
  std::string titleIPM1c2_3 = "Ipm SB1c 3, SB2 Sum#Ipm Correlations #0#1"+colstr[3];
  _corr21_0 = new EntryScan(DescScan(titleIPM12_0.c_str(), "SB2 Sum", "Ipm SB1 Ch_0", 200));
  _corr21_1 = new EntryScan(DescScan(titleIPM12_1.c_str(), "SB2 Sum", "Ipm SB1 Ch_1", 200));
  _corr21_2 = new EntryScan(DescScan(titleIPM12_2.c_str(), "SB2 Sum", "Ipm SB1 Ch_2", 200));
  _corr21_3 = new EntryScan(DescScan(titleIPM12_3.c_str(), "SB2 Sum", "Ipm SB1 Ch_3", 200));
  _corr21c_0 = new EntryScan(DescScan(titleIPM1c2_0.c_str(), "SB2 Sum", "Ipm SB1 Ch_0", 200));
  _corr21c_1 = new EntryScan(DescScan(titleIPM1c2_1.c_str(), "SB2 Sum", "Ipm SB1 Ch_1", 200));
  _corr21c_2 = new EntryScan(DescScan(titleIPM1c2_2.c_str(), "SB2 Sum", "Ipm SB1 Ch_2", 200));
  _corr21c_3 = new EntryScan(DescScan(titleIPM1c2_3.c_str(), "SB2 Sum", "Ipm SB1 Ch_3", 200));
  cds.add(_corr21_0);
  cds.add(_corr21_1);
  cds.add(_corr21_2);
  cds.add(_corr21_3);
  cds.add(_corr21c_0);
  cds.add(_corr21c_1);
  cds.add(_corr21c_2);
  cds.add(_corr21c_3);

  std::string titleIPM1u_0 = "Ipm SB1, User#Ipm Correlations #1#0"+colstr[0];
  std::string titleIPM1u_1 = "Ipm SB1, User#Ipm Correlations #1#0"+colstr[1];
  std::string titleIPM1u_2 = "Ipm SB1, User#Ipm Correlations #1#0"+colstr[2];
  std::string titleIPM1u_3 = "Ipm SB1, User#Ipm Correlations #1#0"+colstr[3];
  std::string titleIPM1cu_0 = "Ipm SB1c, User#Ipm Correlations #1#1"+colstr[0];
  std::string titleIPM1cu_1 = "Ipm SB1c, User#Ipm Correlations #1#1"+colstr[1];
  std::string titleIPM1cu_2 = "Ipm SB1c, User#Ipm Correlations #1#1"+colstr[2];
  std::string titleIPM1cu_3 = "Ipm SB1c, User#Ipm Correlations #1#1"+colstr[3];
  _corruser1_0 = new EntryScan(DescScan(titleIPM1u_0.c_str(), "Ipm SB1", "User0", 200));
  _corruser1_1 = new EntryScan(DescScan(titleIPM1u_1.c_str(), "Ipm SB1", "User1", 200));
  _corruser1_2 = new EntryScan(DescScan(titleIPM1u_2.c_str(), "Ipm SB1", "User2", 200));
  _corruser1_3 = new EntryScan(DescScan(titleIPM1u_3.c_str(), "Ipm SB1", "User3", 200));
  _corruser1c_0 = new EntryScan(DescScan(titleIPM1cu_0.c_str(), "Ipm SB1", "User0", 200));
  _corruser1c_1 = new EntryScan(DescScan(titleIPM1cu_1.c_str(), "Ipm SB1", "User1", 200));
  _corruser1c_2 = new EntryScan(DescScan(titleIPM1cu_2.c_str(), "Ipm SB1", "User2", 200));
  _corruser1c_3 = new EntryScan(DescScan(titleIPM1cu_3.c_str(), "Ipm SB1", "User3", 200));
  cds.add(_corruser1_0);
  cds.add(_corruser1_1);
  cds.add(_corruser1_2);
  cds.add(_corruser1_3);
  cds.add(_corruser1c_0);
  cds.add(_corruser1c_1);
  cds.add(_corruser1c_2);
  cds.add(_corruser1c_3);
    
  std::string titleIPMal1_02 = "Ipm SB1, Channel 0/2#Ipm Alignment #0#0"+colstr[0];
  std::string titleIPMal1_13 = "Ipm SB1, Channel 1/3#Ipm Alignment #0#0"+colstr[1];
  std::string titleIPMal1c_02 = "Ipm SB1c, Channel 0/2#Ipm Alignment #1#0"+colstr[0];
  std::string titleIPMal1c_13 = "Ipm SB1c, Channel 1/3#Ipm Alignment #1#0"+colstr[1];
  _corr1_02 = new EntryScan(DescScan(titleIPMal1_02.c_str(), "Ipm SB1 Ch_A", "Ipm SB1 Ch_B", 200));
  _corr1_13 = new EntryScan(DescScan(titleIPMal1_13.c_str(), "Ipm SB1 Ch_A", "Ipm SB1 Ch_B", 200));
  _corr1c_02 = new EntryScan(DescScan(titleIPMal1c_02.c_str(), "Ipm SB1c Ch_A", "Ipm SB1c Ch_B", 200));
  _corr1c_13 = new EntryScan(DescScan(titleIPMal1c_13.c_str(), "Ipm SB1c Ch_A", "Ipm SB1c Ch_B", 200));
  cds.add(_corr1_02);
  cds.add(_corr1_13);
  cds.add(_corr1c_02);
  cds.add(_corr1c_13);
  cds.add(_corr1_01);
  cds.add(_corr1_23);
  cds.add(_corr1c_01);
  cds.add(_corr1c_23);

  _cds = &cds; 
}

//
//  Analyze data for current event from the pointers we captured
//
bool XPPIpmSB1::accept () 
//bool XPPIpmSB1::analyze  () 
{
  if (_cds) {
    if (_ipm_data[IPM1C]){
      _corr1c_01->addy(_ipm_data[IPM1C]->channel()[1] ,_ipm_data[IPM1C]->channel()[0]);
      _corr1c_23->addy(_ipm_data[IPM1C]->channel()[3] ,_ipm_data[IPM1C]->channel()[2]);
      _corr1c_02->addy(_ipm_data[IPM1C]->channel()[2] ,_ipm_data[IPM1C]->channel()[0]);
      _corr1c_13->addy(_ipm_data[IPM1C]->channel()[3] ,_ipm_data[IPM1C]->channel()[1]);
      _corr1c_02->valid(_clk);
      _corr1c_13->valid(_clk);
      _corr1c_01->valid(_clk);
      _corr1c_23->valid(_clk);
      if (_feegas_data){
	_corrgdet1c_0->addy(_ipm_data[IPM1C]->channel()[0], _feegas_data->f_22_ENRC());
	_corrgdet1c_1->addy(_ipm_data[IPM1C]->channel()[1], _feegas_data->f_22_ENRC());
	_corrgdet1c_2->addy(_ipm_data[IPM1C]->channel()[2], _feegas_data->f_22_ENRC());
	_corrgdet1c_3->addy(_ipm_data[IPM1C]->channel()[3], _feegas_data->f_22_ENRC());
	_corrgdet1c_0->valid(_clk);
	_corrgdet1c_1->valid(_clk);
	_corrgdet1c_2->valid(_clk);
	_corrgdet1c_3->valid(_clk);
      }
      if (_ipm_data[IPM2]){
	_corr21c_0->addy(_ipm_data[IPM1C]->channel()[0], _ipm_data[IPM2]->sum() );
	_corr21c_1->addy(_ipm_data[IPM1C]->channel()[1], _ipm_data[IPM2]->sum() );
	_corr21c_2->addy(_ipm_data[IPM1C]->channel()[2], _ipm_data[IPM2]->sum() );
	_corr21c_3->addy(_ipm_data[IPM1C]->channel()[3], _ipm_data[IPM2]->sum() );
	_corr21c_0->valid(_clk);
	_corr21c_1->valid(_clk);
	_corr21c_2->valid(_clk);
	_corr21c_3->valid(_clk);
      }
      if (_ipm_data[IPMUSER]){
	_corruser1c_0->addy(_ipm_data[IPMUSER]->channel()[0], _ipm_data[IPM1C]->sum() );
	_corruser1c_1->addy(_ipm_data[IPMUSER]->channel()[1], _ipm_data[IPM1C]->sum() );
	_corruser1c_2->addy(_ipm_data[IPMUSER]->channel()[2], _ipm_data[IPM1C]->sum() );
	_corruser1c_3->addy(_ipm_data[IPMUSER]->channel()[3], _ipm_data[IPM1C]->sum() );
	_corruser1c_0->valid(_clk);
	_corruser1c_1->valid(_clk);
	_corruser1c_2->valid(_clk);
	_corruser1c_3->valid(_clk);
      }
    }
    if (_ipm_data[IPM1]){
      _corr1_01->addy(_ipm_data[IPM1]->channel()[1] ,_ipm_data[IPM1]->channel()[0]);
      _corr1_23->addy(_ipm_data[IPM1]->channel()[3] ,_ipm_data[IPM1]->channel()[2]);
      _corr1_02->addy(_ipm_data[IPM1]->channel()[2] ,_ipm_data[IPM1]->channel()[0]);
      _corr1_13->addy(_ipm_data[IPM1]->channel()[3] ,_ipm_data[IPM1]->channel()[1]);
      _corr1_02->valid(_clk);
      _corr1_13->valid(_clk);
      _corr1_01->valid(_clk);
      _corr1_23->valid(_clk);
      if (_feegas_data){
	_corrgdet1_0->addy(_ipm_data[IPM1]->channel()[0], _feegas_data->f_22_ENRC());
	_corrgdet1_1->addy(_ipm_data[IPM1]->channel()[1], _feegas_data->f_22_ENRC());
	_corrgdet1_2->addy(_ipm_data[IPM1]->channel()[2], _feegas_data->f_22_ENRC());
	_corrgdet1_3->addy(_ipm_data[IPM1]->channel()[3], _feegas_data->f_22_ENRC());
	_corrgdet1_0->valid(_clk);
	_corrgdet1_1->valid(_clk);
	_corrgdet1_2->valid(_clk);
	_corrgdet1_3->valid(_clk);
      }
      if (_ipm_data[IPM2]){
	_corr21_0->addy(_ipm_data[IPM1]->channel()[0], _ipm_data[IPM2]->sum() );
	_corr21_1->addy(_ipm_data[IPM1]->channel()[1], _ipm_data[IPM2]->sum() );
	_corr21_2->addy(_ipm_data[IPM1]->channel()[2], _ipm_data[IPM2]->sum() );
	_corr21_3->addy(_ipm_data[IPM1]->channel()[3], _ipm_data[IPM2]->sum() );
	_corr21_0->valid(_clk);
	_corr21_1->valid(_clk);
	_corr21_2->valid(_clk);
	_corr21_3->valid(_clk);
      }
      if (_ipm_data[IPMUSER]){
	_corruser1_0->addy(_ipm_data[IPMUSER]->channel()[0], _ipm_data[IPM1]->sum() );
	_corruser1_1->addy(_ipm_data[IPMUSER]->channel()[1], _ipm_data[IPM1]->sum() );
	_corruser1_2->addy(_ipm_data[IPMUSER]->channel()[2], _ipm_data[IPM1]->sum() );
	_corruser1_3->addy(_ipm_data[IPMUSER]->channel()[3], _ipm_data[IPM1]->sum() );
	_corruser1_0->valid(_clk);
	_corruser1_1->valid(_clk);
	_corruser1_2->valid(_clk);
	_corruser1_3->valid(_clk);
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

extern "C" UserModule* create() { return new XPPIpmSB1; }

extern "C" void destroy(UserModule* p) { delete p; }
