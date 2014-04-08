#include "XppBase.hh"

#include "ami/data/Cds.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/lusi.ddl.h"
#include "pdsdata/psddl/bld.ddl.h"
#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/psddl/cspad2x2.ddl.h"

static const double RDIV=20000;

using namespace Ami;

XppBase::XppBase(const char* name, const char* short_name) :
  _cds(0),
  _fname(name),
  _sname(short_name),
  _cspad_temp_conv(new CspadTemp(RDIV))
{
  clear_data();
}

XppBase::~XppBase() { delete _cspad_temp_conv; }

const char* XppBase::name() const { return _fname; }

void XppBase::clock(const Pds::ClockTime& clk) { _clk=clk; }

void XppBase::clear_data() {
  for (uint ipm=0; ipm<NXppIpms; ipm++) {
    _ipm_data[ipm] = 0;
  }
  _ebeam_data = 0; 
  _feegas_data=0;
  for (int i=0;i<4;i++)
    _cspad_data[i]=0; 
  _cspad2x2_data.clear();
  _cspad2x2.clear();
}

void XppBase::reset(FeatureCache&) {
  clear_data();
}

void XppBase::configure(const Pds::DetInfo&   src,
                        const Pds::TypeId&    type,
                        void*                 payload)
{
  // DAQ IPMs:
  if (type.id()==Pds::TypeId::Id_IpmFexConfig) {
    std::string id = std::string(Pds::DetInfo::name(src));
    printf("%s found DAQ IPM '%s'...",_sname,id.c_str());
    //ipmmono
    if (id == ipm_ids[IPMMONO]) {
      _ipmmono = src;
      _ipm_data[IPMMONO]=0;
    }
    //ipmdec
    else if (id == ipm_ids[IPMDEC]) {
      _ipmdec = src;
      _ipm_data[IPMDEC]=0;
    }
    //ipm2
    else if (id == ipm_ids[IPM2]) {
      _ipm2 = src;
      _ipm_data[IPM2]=0;
    }
    //ipm3
    else if (id == ipm_ids[IPM3]) {
      _ipm3 = src;
      _ipm_data[IPM3]=0;
    }
    // pim3
    else if (id == ipm_ids[PIM3]) {
      _pim3 = src;
      _ipm_data[PIM3]=0;
    }
    // pim4
    else if (id == ipm_ids[PIM4]) {
      _pim4 = src;
      _ipm_data[PIM4]=0;
    }
    //ipmuser
    else if (id == ipm_ids[IPMUSER]) {
      _ipmuser = src;
      _ipm_data[IPMUSER]=0;
    }
    //check wether we have a CsPad
    else if (type.id()==Pds::TypeId::Id_CspadConfig) {
      _cspad = static_cast<const Pds::DetInfo&>(src);
      for (int i=0;i<4;i++)
	_cspad_data[i]=0;
      printf("XPPOther found a CsPad %s\n of version %d",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)), type.version());
      /*
	if (type.version()==5){
	cspad_configtype=5;
	_config5 = *reinterpret_cast<const Pds::CsPad::ConfigV5*>(payload);
	} else if (type.version()==4){
	cspad_configtype=4;
	_config4 = *reinterpret_cast<const Pds::CsPad::ConfigV4*>(payload);
	} else {
	cspad_configtype=3;
	_config3 = *reinterpret_cast<const Pds::CsPad::ConfigV3*>(payload);
	}
      */
    }
  //check wether we have a CsPad 140k
    else if (type.id()==Pds::TypeId::Id_Cspad2x2Config) {    
      _cspad2x2.push_back(static_cast<const Pds::DetInfo&>(src));
      _cspad2x2_data.push_back(0);
      printf("XPPOther found a 140k %s - # %d\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)), static_cast<int>(_cspad2x2_data.size()));
      //   } else {
      //    printf("XPPOther found ALSO %s\n",Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    }  
    else {
      printf("(not used)");
    }
  }
}

void XppBase::configure(const Pds::BldInfo&   src,
                        const Pds::TypeId&    type,
                        void*                 payload)
{
  // Shared IPMs
  if (type.id()==Pds::TypeId::Id_IpmFexConfig) {
    std::string id = std::string(Pds::BldInfo::name(src));
    printf("%s found Shared IPM '%s'...",_sname,id.c_str());
    if (id == ipm_ids[IPM1]) {
      _ipm1 = src;
      _ipm_data[IPM1]=0;
    }
    else if (id == ipm_ids[IPM1C]) {
      _ipm1c = src;
      _ipm_data[IPM1C]=0;
    }
    //ipmmono
    else if (id == ipm_ids[IPMMONO]) {
      _ipmmono = src;
      _ipm_data[IPMMONO]=0;
    }
    //ipmdec
    else if (id == ipm_ids[IPMDEC]) {
      _ipmdec = src;
      _ipm_data[IPMDEC]=0;
    }
    //ipm2
    else if (id == ipm_ids[IPM2]) {
      _ipm2 = src;
      _ipm_data[IPM2]=0;
    }
    //ipm3
    else if (id == ipm_ids[IPM3]) {
      _ipm3 = src;
      _ipm_data[IPM3]=0;
    }
    // pim3
    else if (id == ipm_ids[PIM3]) {
      _pim3 = src;
      _ipm_data[PIM3]=0;
    }
    // pim4
    else if (id == ipm_ids[PIM4]) {
      _pim4 = src;
      _ipm_data[PIM4]=0;
    }
    //ipmuser
    else if (id == ipm_ids[IPMUSER]) {
      _ipmuser = src;
      _ipm_data[IPMUSER]=0;
    }
    else {
      printf("(not used)");
    }
    printf("\n");
  }
  // EBeam BLD:
  else if (type.id()==Pds::TypeId::Id_EBeam) { 
    _ebeam = src;
    _ebeam_data=0;
    printf("%s found %s\n",_sname,Pds::BldInfo::name(src));
  } else if (type.id()==Pds::TypeId::Id_FEEGasDetEnergy) { 
    _feegas = static_cast<const Pds::BldInfo&>(src);
    _feegas_data=0;
    printf("XppBas ound %s\n",Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
  }

}

void XppBase::configure(const Pds::ProcInfo&  src,
                        const Pds::TypeId&    type,
                        void*                 payload)
{
  // Nothing to do
}


//
//  Capture pointer to detector data we want for Pds::DetInfo objects
//
void XppBase::event(const Pds::DetInfo&   src,
                    const Pds::TypeId&    type,
                    const Pds::Damage&    damage,
                    void*                 payload)
{
  if (damage.value()) return;

  // DAQ IPMs
  if (type.id()==Pds::TypeId::Id_IpmFex) {
    //if(matches(src,_ipm4))
    if (_ipmmono == src)
      _ipm_data[IPMMONO] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipmdec == src)
      _ipm_data[IPMDEC] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipm2 == src)
      _ipm_data[IPM2] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipm3 == src)
      _ipm_data[IPM3] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_pim3 == src)
      _ipm_data[PIM3] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_pim4 == src)
      _ipm_data[PIM4] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipmuser == src)
      _ipm_data[IPMUSER] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
  }
  else if (type.id()==Pds::TypeId::Id_CspadElement) {
      //     //const Pds::DetInfo& info = static_cast<const Pds::DetInfo&>(src);
      //     _frame = reinterpret_cast<const Pds::CsPad::ElementV2*>(payload);    
      //     const Pds::Xtc& xtc = *reinterpret_cast<const Pds::Xtc*>(reinterpret_cast<const char*>(_frame)-sizeof(Pds::Xtc));
      //     const Pds::CsPad::ElementHeader* element;
      //     int n_quad = 0;
      //     Pds::CsPad::ElementIterator iter;
      //     if ( cspad_configtype==5){
      //       iter = Pds::CsPad::ElementIterator(_config5, xtc);
      //     } else if ( cspad_configtype==4){
      //       iter = Pds::CsPad::ElementIterator(_config4, xtc);
      //     } else if ( cspad_configtype==3){
      //       iter = Pds::CsPad::ElementIterator(_config3, xtc);
      //     }
      //     while( (element=iter.next()) && n_quad<4 ) {  // loop over elements (quadrants)
      //       _cspad_data[n_quad] = element;
      //       n_quad++;
      //     }
      //     //std::cout<<"looking at the CsPad temp data found quad: "<<n_quad<<std::endl;
         }
    else if (type.id()==Pds::TypeId::Id_Cspad2x2Element) {
      //const Pds::DetInfo& info = static_cast<const Pds::DetInfo&>(src);
      //_cspad2x2[NCs140k] = static_cast<const Pds::DetInfo&>(src);
      //change until cspad140 is implemented in monitoring
      //if (info==_cspad2x2) _cspad2x2_data = reinterpret_cast<const Pds::CsPad2x2::ElementHeader*>(payload);
      //   if (info==_cspad2x2) _cspad2x2_data = reinterpret_cast<const Pds::CsPad::ElementHeader*>(payload);
      for(unsigned index=0; index<_cspad2x2.size(); index++)
	//if (_cspad2x2[index] == src) _cspad2x2_data[index] = reinterpret_cast<const Pds::CsPad::ElementHeader*>(payload);
	if (_cspad2x2[index] == src) _cspad2x2_data[index] = reinterpret_cast<const Pds::CsPad2x2::ElementV1*>(payload);
    }
}

void XppBase::event(const Pds::BldInfo&   src,
                    const Pds::TypeId&    type,
                    const Pds::Damage&    damage,
                    void*                 payload)
{
  if (damage.value()) return;

  // Shared IPMs
  if (type.id()==Pds::TypeId::Id_IpmFex) {
    if (_ipm1 == src) 
      _ipm_data[IPM1] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipm1c == src)
      _ipm_data[IPM1C] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipmmono == src)
      _ipm_data[IPMMONO] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipmdec == src)
      _ipm_data[IPMDEC] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipm2 == src)
      _ipm_data[IPM2] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipm3 == src)
      _ipm_data[IPM3] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_pim3 == src)
      _ipm_data[PIM3] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_pim4 == src)
      _ipm_data[PIM4] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
    else if (_ipmuser == src)
      _ipm_data[IPMUSER] = reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);
  }
  else if (type.id()==Pds::TypeId::Id_EBeam) {
    if (_ebeam == src) _ebeam_data = reinterpret_cast<const Pds::Bld::BldDataEBeamV0*>(payload);
  } 
  else if (type.id()==Pds::TypeId::Id_FEEGasDetEnergy) {
    //const Pds::BldInfo& info = static_cast<const Pds::BldInfo&>(src);
    if (_feegas == src) _feegas_data = reinterpret_cast<const Pds::Bld::BldDataFEEGasDetEnergy*>(payload);
  }
}

void XppBase::event(const Pds::ProcInfo&  src,
                    const Pds::TypeId&    type,
                    const Pds::Damage&    damage,
                    void*                 payload)
{
  // Nothing to do
}
