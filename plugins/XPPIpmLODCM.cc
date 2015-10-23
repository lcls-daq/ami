#include "XPPIpmLODCM.hh"

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

XPPIpmLODCM::XPPIpmLODCM() : 
  XppBase("XPP-IPM-LODCM", "XppIpmLODCMSummary")
{}
XPPIpmLODCM::~XPPIpmLODCM() {}




//
//  Remove all plot entries
//
void XPPIpmLODCM::clear    () 
{
  if (_cds) {

    //std::cout<<"XPPIpmLODCM::clear    "<<std::endl;
    _cds->remove(_mondec[0]);
    _cds->remove(_mondec[1]);
    _cds->remove(_mondec[2]);
    _cds->remove(_mondec[3]);

    _cds->remove(_mondio[0]);
    _cds->remove(_mondio[1]);
    _cds->remove(_corrMonDecDio[0]);
    _cds->remove(_corrMonDecDio[1]);


    _cds = 0;
    _mondec[0]=0;
    _mondec[1]=0;
    _mondec[2]=0;
    _mondec[3]=0;

    _mondio[0]=0;
    _mondio[1]=0;
    _corrMonDecDio[0]=0;
    _corrMonDecDio[1]=0;
  }
}

//
//  Create all plot entries
//
void XPPIpmLODCM::create   (Cds& cds)
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
  printf("XPPIpmLODCM::create plots\n");

  //LODCM channels
  std::string mon2yax = "LODCM Diodes ";// + std::string(1,'0'+char(i));
  std::string mon1yax = "Dectris Channels ";// + std::string(1,'0'+char(i));
  for (int i=0;i<4;i++){
    std::string titleMon1 = "LODCM Dectris "+std::string(1,'0'+char(i))+"#LODCM Diodes#0#0"; 
    std::string titleMon2 = "LODCM Diode "+std::string(1,'0'+char(i))+"#LODCM Diodes#0#1"; 
    _mondec[i] = new EntryScalar(DescScalar((titleMon1+colstr[i]).c_str(),mon1yax.c_str()));
    cds.add(_mondec[i]);
    if (i<2){
      _mondio[i] = new EntryScalar(DescScalar((titleMon2+colstr[i]).c_str(),mon2yax.c_str()));
      cds.add(_mondio[i]);
      std::string titleDecDio = "Dectris Sum vs Diode"+std::string(1,'0'+char(i))+"#LODCM Diodes#0#2";
      _corrMonDecDio[i] = new EntryScan(DescScan((titleDecDio+colstr[i]).c_str(), "LODCM Diode", "Dectris Sum", 200));
      cds.add(_corrMonDecDio[i]); 
    }
  }

  _cds = &cds; 
}

//
//  Analyze data for current event from the pointers we captured
//
bool XPPIpmLODCM::accept () 
{
  
  if (_cds) {
    if (_ipm_data[IPMDEC]){
      //std::cout<<"DECTRIS data: "<<_mon1_data->channel[0]<<" "<<_mon1_data->channel[1]<<" "<<_mon1_data->channel[2]<<" "<<_mon1_data->channel[3]<<std::endl;
      for (int ich=0;ich<4;ich++){
	_mondec[ich]->addcontent(_ipm_data[IPMDEC]->channel()[ich]);
	_mondec[ich]->valid(_clk);
      }
    }
    if (_ipm_data[IPMMONO]){
      for (int ich=0;ich<2;ich++){
	_mondio[ich]->addcontent(_ipm_data[IPMMONO]->channel()[ich]);
	_mondio[ich]->valid(_clk);
	if (_ipm_data[IPMDEC]){
 	  _corrMonDecDio[ich]->addy(_ipm_data[IPMDEC]->sum() ,_ipm_data[IPMMONO]->channel()[ich]);
 	  _corrMonDecDio[ich]->valid(_clk);
	}
      }
    }

  }
  
  //  Reset pointer references
  clear_data();
  return true;
}

extern "C" UserModule* create() { return new XPPIpmLODCM; }

extern "C" void destroy(UserModule* p) { delete p; }
