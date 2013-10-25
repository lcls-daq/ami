#include "UserIpm.hh"

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

UserIpm::UserIpm() :
  XppBase("XPP-USERIPMs", "XppUserIpmSummary")
{}
UserIpm::~UserIpm() {}


//
//  Remove all plot entries
//
void UserIpm::clear    () 
{
  if (_cds) {

    for(unsigned i=0; i<4; i++){
      _cds->remove(_useripm[i]);
      _cds->remove(_SB34pim[i]);
      _cds->remove(_corr2User[i]);
    }
    _cds->remove(_SB34pim[4]);

    _cds = 0;
    for(unsigned i=0; i<4; i++){
      _useripm[i]=0;
      _SB34pim[i]=0;
      _corr2User[i]=0;
    }
    _SB34pim[4]=0;

  }
}

//
//  Create all plot entries
//
void UserIpm::create   (Cds& cds)
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
  printf("UserIpm::create plots\n");
    
  //IPM channels
  for (int i=0;i<4;i++){
    std::string titleIPMU = "User Ipm  "+std::string(1,'0'+char(i))+"#User Ipm Channels#0#0"; 
    std::string titlecorr2 = "User IPM vs SB2, Ch"+std::string(1,'0'+char(i))+"#User Ipm Channels#0#1";
    std::string titlePIM = "SB3 Pim "+std::string(1,'0'+char(i))+"#User Ipm Channels#0#2"; 
    std::string useryax = "User Ipm Channels ";// + std::string(1,'0'+char(i));
    std::string sb4yax = "SB3(4) Pim Channels ";// + std::string(1,'0'+char(i));
    //    _useripm[i] = new EntryScalar(DescScalar((titleIPMU+titlech+colstr[i]).c_str(),useryax.c_str()));
    //    _SB4pim[i] = new EntryScalar(DescScalar((titlePIM4+titlech+colstr[i]).c_str(),sb4yax.c_str()));
    _useripm[i] = new EntryScalar(DescScalar((titleIPMU+colstr[i]).c_str(),useryax.c_str()));
    _SB34pim[i] = new EntryScalar(DescScalar((titlePIM+colstr[i]).c_str(),sb4yax.c_str()));
    _corr2User[i] = new EntryScan(DescScan((titlecorr2+colstr[i]).c_str(), "Ipm SB2 Sum", useryax.c_str(), 200));
    cds.add(_useripm[i]);
    cds.add(_SB34pim[i]);
    cds.add(_corr2User[i]);
  }
  std::string titlePIM4 = "SB4 Pim 0#User Ipm Channels#0#2"; 
  std::string sb4yax = "SB3(4) Pim Channels ";
  _SB34pim[4] = new EntryScalar(DescScalar((titlePIM4+colstr[4]).c_str(),sb4yax.c_str()));
  cds.add(_SB34pim[4]);

  _cds = &cds; 
}

//
//  Analyze data for current event from the pointers we captured
//
bool UserIpm::accept () 
//bool UserIpm::analyze  () 
{
//   printf("UserIpm::accept _sb2_data(%p), _sb3_data(%p)  _sb2[%08x.%08x]\n",
//          _sb2_data, _sb3_data, _sb2.log(), _sb2.phy());

  if (_cds) {
    if (_ipm_data[PIM3]){
      for (int ich=0;ich<4;ich++){
	_SB34pim[ich]->addcontent(_ipm_data[PIM3]->channel()[ich]);
	_SB34pim[ich]->valid(_clk);
      }
    }
    if (_ipm_data[PIM4]){
      _SB34pim[4]->addcontent(_ipm_data[PIM4]->channel()[0]);
      _SB34pim[4]->valid(_clk);
    }
    if (_ipm_data[IPMUSER]){
      for (int ich=0;ich<4;ich++){
	_useripm[ich]->addcontent(_ipm_data[IPMUSER]->channel()[ich]);
	_useripm[ich]->valid(_clk);
	if (_ipm_data[IPM2]){
	  _corr2User[ich]->addy(_ipm_data[IPMUSER]->channel()[ich],_ipm_data[IPM2]->sum());
	  _corr2User[ich]->valid(_clk);
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

extern "C" UserModule* create() { return new UserIpm; }

extern "C" void destroy(UserModule* p) { delete p; }
