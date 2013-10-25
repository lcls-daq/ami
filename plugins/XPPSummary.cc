#include "XPPSummary.hh"

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
#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/psddl/cspad2x2.ddl.h"

#include <string>
#include <iostream>

using namespace Ami;
using namespace std;

XPPSummary::XPPSummary() : _cds(0) {}
XPPSummary::~XPPSummary() {}

const char* XPPSummary::name() const { return "XPPPlots"; }
void XPPSummary::clock    (const Pds::ClockTime& clk) { _clk=clk; }

void XPPSummary::reset    (FeatureCache&) { 
_sb1_data=0;
_sb1c_data=0;
_sb2_data=0; 
_sb3_data=0; 
_sb3p_data=0; 
_sb4p_data=0; 
_mon1_data=0; 
_mon2_data=0; 
_ipmuser_data=0; 
_ebeam_data=0; 
_phasecav_data=0; 
_feegas_data=0; 
for (int i=0;i<4;i++)
  _cspad_data[i]=0; 
 _cspad2x2_data.clear();
 _cspad2x2.clear();
}

void XPPSummary::_configure(const Pds::Src&       src,
                           const Pds::TypeId&    type,
                           void*                 payload) 
{
  //check wether we have an ipm
  if (type.id()==Pds::TypeId::Id_IpmFexConfig) {
    if ((src.level()==Pds::Level::Source   && std::string(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)))=="XppSb2Ipm-1|Ipimb-0") ||
        (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppSb2_Ipm")) {
      _sb2 = src;
      _sb2_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else if ((src.level()==Pds::Level::Source   && std::string(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)))=="XppSb3Ipm-1|Ipimb-0") ||
	       (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppSb3_Ipm")) {
      _sb3 = src;
      _sb3_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else if ((src.level()==Pds::Level::Source   && std::string(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)))=="XppSb3Pim-1|Ipimb-0") ||
	       (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppSb3_Pim")) {
      _sb3p = src;
      _sb3p_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else if ((src.level()==Pds::Level::Source   && std::string(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)))=="XppSb4Pim-1|Ipimb-0") ||
	       (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppSb4_Pim")) {
      _sb4p = src;
      _sb4p_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else if ((src.level()==Pds::Level::Source   && std::string(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)))=="XppMonPim-1|Ipimb-0") ||
	       (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppMon_Pim0")) {
      _mon1 = src;
      _mon1_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else if ((src.level()==Pds::Level::Source   && std::string(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)))=="XppMonPim-1|Ipimb-1") ||
	       (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppMon_Pim1")) {
      _mon2 = src;
      _mon2_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else if ((src.level()==Pds::Level::Source   && std::string(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)))=="XppEndstation-1|Ipimb-0") ||
	       (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppEnds_Ipm0")) {
      _ipmuser = src;
      _ipmuser_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="NH2-SB1-IPM-01") {
      _sb1 = src;
      _sb1_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="NH2-SB1-IPM-02") {
      _sb1c = src;
      _sb1c_data=0;
      printf("XPPSummary found %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    } else {
      printf("XPPSummary found ALSO the DAQ IPM: %s [%s]\n",
             Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)),
             Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    }
  }
  else if (type.id()==Pds::TypeId::Id_SharedIpimb) {
    printf("XPPSummary--SharedIpimb: %s",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="NH2-SB1-IPM-01") {
      _sb1 = src;
      _sb1_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="NH2-SB1-IPM-02") {
      _sb1c = src;
      _sb1c_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppMon_Pim0") {
      _mon1 = src;
      _mon1_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppMon_Pim1") {
      _mon2 = src;
      _mon2_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppSb2_Ipm") {
      _sb2 = src;
      _sb2_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppSb3_Ipm") {
      _sb3 = src;
      _sb3_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppSb3_Pim") {
      _sb3p = src;
      _sb3p_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppSb4_Pim") {
      _sb4p = src;
      _sb4p_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else if (src.level()==Pds::Level::Reporter && std::string(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)))=="XppEnds_Ipm0") {
      _ipmuser = src;
      _ipmuser_data=0;
      printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    } else {
      printf("XPPSummary found ALSO the SharedIpimb: %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    }
  }
  else if (type.id()==Pds::TypeId::Id_EBeam) { 
    _ebeam = static_cast<const Pds::BldInfo&>(src);
    _ebeam_data=0;
    printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
  }
  else if (type.id()==Pds::TypeId::Id_PhaseCavity) { 
    _phasecav = static_cast<const Pds::BldInfo&>(src);
    _phasecav_data=0;
    printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
  }
  else if (type.id()==Pds::TypeId::Id_FEEGasDetEnergy) { 
    _feegas = static_cast<const Pds::BldInfo&>(src);
    _feegas_data=0;
    printf("XPPSummary found %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
  }
  //check wether we have a CsPad
  else if (type.id()==Pds::TypeId::Id_CspadConfig) {
    _cspad = static_cast<const Pds::DetInfo&>(src);
    for (int i=0;i<4;i++)
      _cspad_data[i]=0;
    printf("XPPSummary found a CsPad %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    //    _config = *reinterpret_cast<const Pds::CsPad::ConfigV3*>(payload);
    _config = *reinterpret_cast<const Pds::CsPad::ConfigV5*>(payload); // dflath 2013-06-05
  }
  //check wether we have a CsPad 140k
  else if (type.id()==Pds::TypeId::Id_Cspad2x2Config) {    
    _cspad2x2.push_back(static_cast<const Pds::DetInfo&>(src));
    _cspad2x2_data.push_back(0);
    printf("XPPSummary found a 140k %s - # %d\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)), _cspad2x2_data.size());
    //   } else {
    //    printf("XPPSummary found ALSO %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
  }
  _cspad_temp_conv->instance();
}

bool matches(const Pds::Src& a, const Pds::Src& b)
{
  return a.level()==b.level() && a.phy()==b.phy();
}

//
//  Capture pointer to detector data we want
//
void XPPSummary::_event    (const Pds::Src&       src,
                           const Pds::TypeId&    type,
                           void*                 payload) 
{
  if (type.id()==Pds::TypeId::Id_IpmFex) {
//     printf("event %08x.%08x  [sb2 %08x.%08x  sb3 %08x.%08x]\n",
//            src.log(),src.phy(),
//            _sb2.log(),_sb2.phy(),
//            _sb3.log(),_sb3.phy());
    //const Pds::DetInfo& info = static_cast<const Pds::DetInfo&>(src);
    if (matches(src,_sb1)) _sb1_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    if (matches(src,_sb1c)) _sb1c_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    if (matches(src,_sb2)) _sb2_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    if (matches(src,_sb3)) _sb3_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    if (matches(src,_sb3p)) _sb3p_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    if (matches(src,_sb4p)) _sb4p_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    if (matches(src,_mon1)) _mon1_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    if (matches(src,_mon2)) _mon2_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    if (matches(src,_ipmuser)) _ipmuser_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
//     if (src==_sb2) _sb2_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
//     if (src==_sb3) _sb3_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
//     if (src==_sb4) _sb4_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
//     if (src==_mon1) _mon1_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
//     if (src==_mon2) _mon2_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
//     if (src==_ipmuser) _ipmuser_data = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
  }
  else if (type.id()==Pds::TypeId::Id_SharedIpimb) {
    const Pds::BldInfo& info = static_cast<const Pds::BldInfo&>(src);
    if (matches(info,_sb1)) _sb1_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
    if (matches(info,_sb1c)) _sb1c_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
    if (matches(info,_mon1)) _mon1_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
    if (matches(info,_mon2)) _mon2_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
    if (matches(info,_sb2)) _sb2_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
    if (matches(info,_sb3)) _sb3_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
    if (matches(info,_sb3p)) _sb3p_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
    if (matches(info,_sb4p)) _sb4p_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
    if (matches(info,_ipmuser)) _ipmuser_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
//     if (info==_sb1) _sb1_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
//     if (info==_mon1) _mon1_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
//     if (info==_mon2) _mon2_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
//     if (info==_sb2) _sb2_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
//     if (info==_sb3) _sb3_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
//     if (info==_sb4) _sb4_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
//     if (info==_ipmuser) _ipmuser_data = &reinterpret_cast<const Pds::Bld::BldDataIpimbV1*>(payload)->ipmFexData();
  }
  else if (type.id()==Pds::TypeId::Id_EBeam) {
    const Pds::BldInfo& info = static_cast<const Pds::BldInfo&>(src);
    if (matches(info,_ebeam)) _ebeam_data = reinterpret_cast<const Pds::Bld::BldDataEBeamV0*>(payload);
    //    if (info==_ebeam) _ebeam_data = reinterpret_cast<const Pds::Bld::BldDataEBeamV0*>(payload);
  }
  else if (type.id()==Pds::TypeId::Id_PhaseCavity) {
    const Pds::BldInfo& info = static_cast<const Pds::BldInfo&>(src);
    if (matches(info,_phasecav)) _phasecav_data = reinterpret_cast<const Pds::Bld::BldDataPhaseCavity*>(payload);
    //    if (info==_phasecav) _phasecav_data = reinterpret_cast<const Pds::Bld::BldDataPhaseCavity*>(payload);
  }
  else if (type.id()==Pds::TypeId::Id_FEEGasDetEnergy) {
    const Pds::BldInfo& info = static_cast<const Pds::BldInfo&>(src);
    if (matches(info,_feegas)) _feegas_data = reinterpret_cast<const Pds::Bld::BldDataFEEGasDetEnergy*>(payload);
    //    if (info==_feegas) _feegas_data = reinterpret_cast<const Pds::Bld::BldDataFEEGasDetEnergy*>(payload);
  }
  else if (type.id()==Pds::TypeId::Id_CspadElement) {
    //const Pds::DetInfo& info = static_cast<const Pds::DetInfo&>(src);
    _frame = reinterpret_cast<const Pds::CsPad::ElementV2*>(payload);    
    for(unsigned i=0; i<_config.numQuads(); i++)
      _cspad_data[i] = &reinterpret_cast<const Pds::CsPad::DataV2*>(payload)->quads(_config,i);
    //std::cout<<"looking at the CsPad temp data found quad: "<<n_quad<<std::endl;
  }
  else if (type.id()==Pds::TypeId::Id_Cspad2x2Element) {
    const Pds::DetInfo& info = static_cast<const Pds::DetInfo&>(src);
    //_cspad2x2[NCs140k] = static_cast<const Pds::DetInfo&>(src);
    //change until cspad140 is implemented in monitoring
    //if (info==_cspad2x2) _cspad2x2_data = reinterpret_cast<const Pds::CsPad2x2::ElementHeader*>(payload);
    //   if (info==_cspad2x2) _cspad2x2_data = reinterpret_cast<const Pds::CsPad::ElementHeader*>(payload);
    for(unsigned index=0; index<_cspad2x2.size(); index++)
      if (matches(info,_cspad2x2[index])) _cspad2x2_data[index] = reinterpret_cast<const Pds::CsPad2x2::ElementV1*>(payload);
  }
}

//
//  Remove all plot entries
//
void XPPSummary::clear    () 
{
  if (_cds) {
    _cds->remove(_correbgas);
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
    _cds->remove(_ebeam_sb2);
    _cds->remove(_phasecav_t12);
    _cds->remove(_th1f_sb2);
    _cds->remove(_th1f_sb3);

    for(unsigned i=0; i<4; i++){
      _cds->remove(_useripm[i]);
      _cds->remove(_SB34pim[i]);
      _cds->remove(_cspad_temp[i]);

      _cds->remove(_corr2_c1[i]);
      _cds->remove(_corr2_c3[i]);

      for(unsigned j=0; j<4; j++)
	_cds->remove(_cspad2x2_temp[i][j]);
      _cds->remove(_corr2User[i]);
      _cds->remove(_mondec[i]);
      if (i<2){
	_cds->remove(_mondio[i]);
	_cds->remove(_corrMonDecDio[i]);
      }
    }
    _cds->remove(_SB34pim[4]);

    _cds = 0;
    _correbgas=0;
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
    _ebeam_sb2=0;
    _phasecav_t12=0;
    for(unsigned i=0; i<4; i++){
      _useripm[i]=0;
      _SB34pim[i]=0;
      _cspad_temp[i]=0;

      _corr2_c1[i]=0;
      _corr2_c3[i]=0;

      for(unsigned j=0; j<4; j++)
	_cspad2x2_temp[i][j]=0;
      _corr2User[i]=0;
      _mondec[i]=0;
      if (i<2){
	_mondio[i]=0;
	_corrMonDecDio[i]=0;
      }
    }
    _SB34pim[4]=0;
    _th1f_sb2=0;
    _th1f_sb3=0;

  }
}

//
//  Create all plot entries
//
void XPPSummary::create   (Cds& cds)
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
  printf("XPPSummary::create plots\n");
  
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
  
  //IPM channels
  for (int i=0;i<4;i++){
    std::string titlePIM = "SB3 Pim "+std::string(1,'0'+char(i))+"#User Ipm Channels#0#0"; 
    std::string titleIPMU = "User Ipm  "+std::string(1,'0'+char(i))+"#User Ipm Channels#0#1"; 
    std::string titlecorr2 = "User IPM vs SB2, Ch"+std::string(1,'0'+char(i))+"#User Ipm Channels#0#2";
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
  std::string titlePIM4 = "SB4 Pim 0#User Ipm Channels#0#0"; 
  std::string sb4yax = "SB3(4) Pim Channels ";
  _SB34pim[4] = new EntryScalar(DescScalar((titlePIM4+colstr[4]).c_str(),sb4yax.c_str()));
  cds.add(_SB34pim[4]);
  //LODCM channels
  for (int i=0;i<4;i++){
    std::string titleMon1 = "LODCM Dectris "+std::string(1,'0'+char(i))+"#LODCM Diodes#0#0"; 
    std::string titleMon2 = "LODCM Diode "+std::string(1,'0'+char(i))+"#LODCM Diodes#0#1"; 
    std::string mon2yax = "LODCM Diodes ";// + std::string(1,'0'+char(i));
    std::string mon1yax = "Dectris Channels ";// + std::string(1,'0'+char(i));
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
bool XPPSummary::accept () 
//bool XPPSummary::analyze  () 
{
//   printf("XPPSummary::accept _sb2_data(%p), _sb3_data(%p)  _sb2[%08x.%08x]\n",
//          _sb2_data, _sb3_data, _sb2.log(), _sb2.phy());

  if (_cds) {
    if (_sb2_data){
      _scan_sb2->addcontent(_sb2_data->sum());
      _corr2_02->addy(_sb2_data->channel()[2] ,_sb2_data->channel()[0]);
      _corr2_13->addy(_sb2_data->channel()[3] ,_sb2_data->channel()[1]);
      _corr2_01->addy(_sb2_data->channel()[1] ,_sb2_data->channel()[0]);
      _corr2_23->addy(_sb2_data->channel()[3] ,_sb2_data->channel()[2]);
      _th1f_sb2->addcontent(1,_sb2_data->sum());
      _scan_sb2->valid(_clk);
      _corr2_02->valid(_clk);
      _corr2_13->valid(_clk);
      _corr2_01->valid(_clk);
      _corr2_23->valid(_clk);
      _th1f_sb2->valid(_clk);
    }
    if (_sb3_data){
      _scan_sb3->addcontent(_sb3_data->sum());
      _corr3_02->addy(_sb3_data->channel()[2] ,_sb3_data->channel()[0]);
      _corr3_13->addy(_sb3_data->channel()[3] ,_sb3_data->channel()[1]);
      _corr3_01->addy(_sb3_data->channel()[1] ,_sb3_data->channel()[0]);
      _corr3_23->addy(_sb3_data->channel()[3] ,_sb3_data->channel()[2]);
      _th1f_sb3->addcontent(1,_sb3_data->sum());
      _scan_sb3->valid(_clk);
      _corr3_02->valid(_clk);
      _corr3_13->valid(_clk);
      _corr3_01->valid(_clk);
      _corr3_23->valid(_clk);
      _th1f_sb3->valid(_clk);
    }

    if (_sb2_data && _sb3_data){
      _corr23->addy(_sb3_data->sum() ,_sb2_data->sum() );
      _corr23->valid(_clk);

      for (int ich=0;ich<4;ich++){
	_corr2_c3[ich]->addy(_sb3_data->channel()[ich] ,_sb2_data->sum() );
	_corr2_c3[ich]->valid(_clk);
      }
    }
    if (_sb1_data){
      _corr1_02->addy(_sb1_data->channel()[2] ,_sb1_data->channel()[0]);
      _corr1_13->addy(_sb1_data->channel()[3] ,_sb1_data->channel()[1]);
      _corr1_01->addy(_sb1_data->channel()[1] ,_sb1_data->channel()[0]);
      _corr1_23->addy(_sb1_data->channel()[3] ,_sb1_data->channel()[2]);
      if (_sb2_data){
	for (int ich=0;ich<4;ich++){
	  _corr2_c1[ich]->addy(_sb1_data->channel()[ich] ,_sb2_data->sum() );
	  _corr2_c1[ich]->valid(_clk);
	}
      }
    }
    if (_feegas_data){
      _scan_gasdet->addcontent(_feegas_data->f_11_ENRC());
      _scan_gasdet->valid(_clk);
    }
    if (_sb3p_data){
      for (int ich=0;ich<4;ich++){
	_SB34pim[ich]->addcontent(_sb3p_data->channel()[ich]);
	_SB34pim[ich]->valid(_clk);
      }
    }
    if (_sb4p_data){
      _SB34pim[4]->addcontent(_sb4p_data->channel()[0]);
      _SB34pim[4]->valid(_clk);
    }
    if (_ipmuser_data){
      for (int ich=0;ich<4;ich++){
	_useripm[ich]->addcontent(_ipmuser_data->channel()[ich]);
	_useripm[ich]->valid(_clk);
	if (_sb2_data){
	  _corr2User[ich]->addy(_ipmuser_data->channel()[ich],_sb2_data->sum());
	  _corr2User[ich]->valid(_clk);
	}
      }
    }

    if (_phasecav_data) {
      _phasecav_t12->addy(_phasecav_data->fitTime2(), _phasecav_data->fitTime1());
      _phasecav_t12->valid(_clk);
    }
    if (_ebeam_data) {
      if (_sb2_data) {
	_ebeam_sb2->addy(_sb2_data->sum(),_ebeam_data->ebeamL3Energy());
	_ebeam_sb2->valid(_clk);
      }
      if (_feegas_data){
	_correbgas->addy(_feegas_data->f_11_ENRC(),_ebeam_data->ebeamL3Energy());
	_correbgas->valid(_clk);
      }
    }

    if (_mon1_data){
      //std::cout<<"DECTRIS data: "<<_mon1_data->channel()[0]<<" "<<_mon1_data->channel()[1]<<" "<<_mon1_data->channel()[2]<<" "<<_mon1_data->channel()[3]<<std::endl;
      for (int ich=0;ich<4;ich++){
	_mondec[ich]->addcontent(_mon1_data->channel()[ich]);
	_mondec[ich]->valid(_clk);
      }
    }
    if (_mon2_data){
      for (int ich=0;ich<2;ich++){
	_mondio[ich]->addcontent(_mon2_data->channel()[ich]);
	_mondio[ich]->valid(_clk);
	if (_mon1_data){
 	  _corrMonDecDio[ich]->addy(_mon1_data->sum() ,_mon2_data->channel()[ich]);
 	  _corrMonDecDio[ich]->valid(_clk);
 	  if ((_mon2_data->channel()[0]+_mon2_data->channel()[1])<-15.||_mon1_data->sum()<-15.) std::cout<<_mon1_data->sum()<<" "<<_mon1_data->channel()[0]+_mon1_data->channel()[1]+_mon1_data->channel()[2]+_mon1_data->channel()[3]<<" mon2 "<<_mon2_data->channel()[0]<<" "<<_mon2_data->channel()[1]<<std::endl;
	}
      }
    }

//     if (_cspad_data){
//       for (int ich=0;ich<4;ich++){
// 	//for (int ich=0;ich<1;ich++){
// 	//with the new(as of Mar 2012) CsPad, only the 4th thermocouple of each quad reads something
// 	std::cout<<ich<<" --- temp: "<<_cspad_data->sb_temp(ich*4+3)<<std::endl;
// 	_cspad_temp[ich]->addcontent(_cspad_temp_conv->getTemp(_cspad_data->sb_temp(ich*4+3)));
//  	_cspad_temp[ich]->valid(_clk);
//       }
//     }
    for (int ich=0;ich<4;ich++){
      if (_cspad_data[ich]){
	//std::cout<<ich<<" temp: "<<_cspad_data[ich]->sb_temp()[0]<<" "<<_cspad_data[ich]->sb_temp()[1]<<" "<<_cspad_data[ich]->sb_temp()[2]<<" "<<_cspad_data[ich]->sb_temp()[3]<<" "<<std::endl;
	if (_cspad_data[ich]->sb_temp()[3] >  _cspad_data[ich]->sb_temp()[0] ) {
	  _cspad_temp[ich]->addcontent(_cspad_temp_conv->getTemp(_cspad_data[ich]->sb_temp()[3]));
	} else {
	  _cspad_temp[ich]->addcontent(_cspad_temp_conv->getTemp(_cspad_data[ich]->sb_temp()[0]));
	}
  	_cspad_temp[ich]->valid(_clk);
      }
    }
    for (unsigned idet=0;idet<_cspad2x2_data.size();idet++){
      if (_cspad2x2_data[idet]){
	for (int ich=0;ich<4;ich++){
	  //change until cspad140 is implemented in monitoring
	  //_cspad2x2_temp->addcontent(_cspad_temp_conv->getTemp(_cspad2x2_data->sb_temp()[ich]));
	  //TODO: only plot if temp > 10C (assume otherwise it's not real...
	  if (_cspad2x2_data[idet]->sb_temp()[ich] > 10){
          _cspad2x2_temp[ich][idet]->addcontent(_cspad_temp_conv->getTemp(_cspad2x2_data[idet]->sb_temp()[ich]));
	    _cspad2x2_temp[ich][idet]->valid(_clk);
	  }
	  //cout<<"CsPad 140k temps: "<<_cspad2x2_data->sb_temp()[0]<<" "<<_cspad2x2_data->sb_temp()[1]<<" "<<_cspad2x2_data->sb_temp()[2]<<" "<<_cspad2x2_data->sb_temp()[3]<<endl;
	}
      }
    }
  }
  //  Reset pointer references
  _sb1_data = 0;
  _sb2_data = 0;
  _sb3_data = 0;
  _sb4p_data = 0;
  _mon1_data = 0;
  _mon2_data = 0;
  _ipmuser_data = 0;
  _ebeam_data = 0;
  _phasecav_data = 0;
  for (int i=0;i<4;i++)
    _cspad_data[i] = 0;
  for (unsigned i=0;i<_cspad2x2_data.size();i++)
    _cspad2x2_data[i] = 0;

  return true;
}

bool XPPSummary::analyze  () 
//bool XPPSummary::accept () 
 {
   //std::cout<<"does this get called - analyze? "<<std::endl;
    return true;
 }
//
//  Plug-in module creator
//

extern "C" UserModule* create() { return new XPPSummary; }

extern "C" void destroy(UserModule* p) { delete p; }
