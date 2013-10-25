#include "XPPOther.hh"

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
#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/psddl/cspad2x2.ddl.h"
// #include "pdsdata/cspad/ElementHeader.hh"
// #include "pdsdata/cspad/ElementIterator.hh"
// #include "pdsdata/cspad2x2/ConfigV1.hh"
// #include "pdsdata/cspad2x2/ElementHeader.hh"

#include <string>
#include <iostream>

using namespace Ami;
using namespace std;

XPPOther::XPPOther() : 
  XppBase("XPP-OTHER", "XppOtherSummary")
{}
XPPOther::~XPPOther() {}




//
//  Remove all plot entries
//
void XPPOther::clear    () 
{
  if (_cds) {
    for(unsigned i=0; i<4; i++){
      _cds->remove(_cspad_temp[i]);

      _cds->remove(_corr2_c1[i]);
      _cds->remove(_corr2_c3[i]);
    }

    _cds = 0;
    for(unsigned i=0; i<4; i++){
      _cspad_temp[i]=0;

      _corr2_c1[i]=0;
      _corr2_c3[i]=0;

      for(unsigned j=0; j<4; j++)
	_cspad2x2_temp[i][j]=0;
    }
  }
}

//
//  Create all plot entries
//
void XPPOther::create   (Cds& cds)
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
  printf("XPPOther::create plots\n");
        
// CsPad temperatures
   for (int i=0;i<4;i++){
     std::string titletcspad = "CsPad Quad temp "+std::string(1,'0'+char(i))+"#CsPad temps#0#0"; 
     std::string yax = "CsPad Temp ";// + std::string(1,'0'+char(i));
     //     _cspad_temp[i] = new EntryScalar(DescScalar((titletcspad+titlech[i]+colstr[i]).c_str(),yax.c_str()));
     _cspad_temp[i] = new EntryScalar(DescScalar((titletcspad+colstr[i]).c_str(),yax.c_str()));
     cds.add(_cspad_temp[i]);
   }

   for (int i=0;i<4;i++){
     for (int j=0;j<4;j++){
       std::string titletcspad2x2 = "140k "+string(1,'0'+char(j))+" temp "+string(1,'0'+char(i))+"#CsPad temps#0#"+string(1,'0'+char(j%2+1)); 
       std::string yax = "CsPad 140k Temp ";// + std::string(1,'0'+char(i));
       //     _cspad2x2_temp[i] = new EntryScalar(DescScalar((titletcspad2x2+titlech[i]+colstr[i]).c_str(),yax.c_str()));
       _cspad2x2_temp[i][j] = new EntryScalar(DescScalar((titletcspad2x2+colstr[i+(j%2)*4]).c_str(),yax.c_str()));
       cds.add(_cspad2x2_temp[i][j]);
     }
   }

  //IPM channels
  for (int i=0;i<4;i++){
    std::string titlecorr2_c1 = "SB1 IPM vs SB2, Ch"+std::string(1,'0'+char(i))+"#SB1_3 Debug#0#0";
    std::string titlecorr2_c3 = "SB3 IPM vs SB2, Ch"+std::string(1,'0'+char(i))+"#SB1_3 Debug#0#1";
    //debug plots for IPM on SB3 + SB1 (correlation w/ SB2)
    _corr2_c1[i] = new EntryScan(DescScan((titlecorr2_c1+colstr[i]).c_str(),"IPM SB2 Sum","SB1 channel",200));
    _corr2_c3[i] = new EntryScan(DescScan((titlecorr2_c3+colstr[i]).c_str(),"IPM SB2 Sum","SB3 channel",200));
    cds.add(_corr2_c1[i]);
    cds.add(_corr2_c3[i]);
  }

  _cds = &cds; 
}

//
//  Analyze data for current event from the pointers we captured
//
bool XPPOther::accept () 
//bool XPPOther::analyze  () 
{
//   printf("XPPOther::accept _sb2_data(%p), _sb3_data(%p)  _sb2[%08x.%08x]\n",
//          _sb2_data, _sb3_data, _sb2.log(), _sb2.phy());
if (_cds) {
    if (_ipm_data[IPM2] && _ipm_data[IPM3]){
      for (int ich=0;ich<4;ich++){
	_corr2_c3[ich]->addy(_ipm_data[IPM3]->channel()[ich] ,_ipm_data[IPM2]->sum() );
	_corr2_c3[ich]->valid(_clk);
      }
    }
    if (_ipm_data[IPM1]){
      if (_ipm_data[IPM2]){
	for (int ich=0;ich<4;ich++){
	  _corr2_c1[ich]->addy(_ipm_data[IPM1]->channel()[ich] ,_ipm_data[IPM2]->sum() );
	  _corr2_c1[ich]->valid(_clk);
	}
      }
    }
//     if (_feegas_data){
//     }
//     if (_phasecav_data) {
//     }

//     if (_cspad_data){
//       for (int ich=0;ich<4;ich++){
// 	//for (int ich=0;ich<1;ich++){
// 	//with the new(as of Mar 2012) CsPad, only the 4th thermocouple of each quad reads something
// 	std::cout<<ich<<" --- temp: "<<_cspad_data->sb_temp(ich*4+3)<<std::endl;
// 	_cspad_temp[ich]->addcontent(_cspad_temp_conv->getTemp(_cspad_data->sb_temp(ich*4+3)));
//  	_cspad_temp[ich]->valid(_clk);
//       }
//     }
/*
    for (int ich=0;ich<4;ich++){
      if (_cspad_data[ich]){
	//std::cout<<ich<<" temp: "<<_cspad_data[ich]->sb_temp(0)<<" "<<_cspad_data[ich]->sb_temp(1)<<" "<<_cspad_data[ich]->sb_temp(2)<<" "<<_cspad_data[ich]->sb_temp(3)<<" "<<std::endl;
	if (_cspad_data[ich]->sb_temp(3) >  _cspad_data[ich]->sb_temp(0) ) {
	  _cspad_temp[ich]->addcontent(_cspad_temp_conv->getTemp(_cspad_data[ich]->sb_temp(3)));
	} else {
	  _cspad_temp[ich]->addcontent(_cspad_temp_conv->getTemp(_cspad_data[ich]->sb_temp(0)));
	}
  	_cspad_temp[ich]->valid(_clk);
      }
    }
*/
    for (int idet=0;idet<static_cast<int>(_cspad2x2_data.size());idet++){
      if (_cspad2x2_data[idet]){
	for (int ich=0;ich<4;ich++){
	  //change until cspad140 is implemented in monitoring
	  //_cspad2x2_temp->addcontent(_cspad_temp_conv->getTemp(_cspad2x2_data->sb_temp(ich)));
	  //TODO: only plot if temp > 10C (assume otherwise it's not real...
	  if (_cspad2x2_data[idet]->sb_temp()[ich] > 10){
	    _cspad2x2_temp[ich][idet]->addcontent(_cspad_temp_conv->getTemp(_cspad2x2_data[idet]->sb_temp()[ich]));
	    _cspad2x2_temp[ich][idet]->valid(_clk);
	  }
	  //cout<<"CsPad 140k temps: "<<_cspad2x2_data->sb_temp(0)<<" "<<_cspad2x2_data->sb_temp(1)<<" "<<_cspad2x2_data->sb_temp(2)<<" "<<_cspad2x2_data->sb_temp(3)<<endl;
	}
      }
    }
  }
  //  Reset pointer references
  clear_data();

  return true;
}

extern "C" UserModule* create() { return new XPPOther; }

extern "C" void destroy(UserModule* p) { delete p; }
