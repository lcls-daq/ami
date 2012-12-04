#include "ami/event/OrcaHandler.hh"
#include "pds/config/OrcaConfigType.hh"
#include "pds/config/FrameFexConfigType.hh"

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_OrcaConfig);
  return types;
}


OrcaHandler::OrcaHandler(const Pds::DetInfo& info) : 
  FrameHandler(info, 
               config_type_list(),
               OrcaConfigType::Row_Pixels,
               OrcaConfigType::Column_Pixels) 
{
}

void OrcaHandler::_configure(Pds::TypeId tid, 
			     const void* payload, const Pds::ClockTime& t)
{
  switch(tid.id()) {
  case Pds::TypeId::Id_OrcaConfig:
    switch(tid.version()) {
    case 1: {
      const Pds::Orca::ConfigV1& c = *reinterpret_cast<const Pds::Orca::ConfigV1*>(payload);
      switch(c.mode()) {
      case Pds::Orca::ConfigV1::x1      : 
	_defRows    = Pds::Orca::ConfigV1::Row_Pixels/1;
	_defColumns = Pds::Orca::ConfigV1::Column_Pixels/1;
	break;
      case Pds::Orca::ConfigV1::x2      : 
	_defRows    = Pds::Orca::ConfigV1::Row_Pixels/2;
	_defColumns = Pds::Orca::ConfigV1::Column_Pixels/2;
	break;
      case Pds::Orca::ConfigV1::x4      : 
	_defRows    = Pds::Orca::ConfigV1::Row_Pixels/4;
	_defColumns = Pds::Orca::ConfigV1::Column_Pixels/4;
	break;
      case Pds::Orca::ConfigV1::Subarray: 
	_defRows    = c.rows();
	_defColumns = Pds::Orca::ConfigV1::Column_Pixels;
	break;
      default:
	break;
      }
    } break;
    default: break;
    } break;
  default:
    FrameHandler::_configure(tid,payload,t);
    break;
  }
}
