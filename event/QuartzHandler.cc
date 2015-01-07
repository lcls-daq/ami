#include "ami/event/QuartzHandler.hh"
#include "pdsdata/psddl/quartz.ddl.h"

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_PimImageConfig);
  types.push_back(Pds::TypeId::Id_QuartzConfig);
  return types;
}


QuartzHandler::QuartzHandler(const Pds::DetInfo& info) : 
  FrameHandler(info, 
               config_type_list(),
               Pds::Quartz::ConfigV1::Column_Pixels,
               Pds::Quartz::ConfigV1::Row_Pixels)
{
}

void QuartzHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_QuartzConfig) {
    const Pds::Quartz::ConfigV1& c = *reinterpret_cast<const Pds::Quartz::ConfigV1*>(payload);
    switch(c.horizontal_binning()) {
    case Pds::Quartz::ConfigV1::x1: _defColumns = Pds::Quartz::ConfigV1::Column_Pixels  ; break;
    case Pds::Quartz::ConfigV1::x2: _defColumns = Pds::Quartz::ConfigV1::Column_Pixels/2; break;
    case Pds::Quartz::ConfigV1::x4: _defColumns = Pds::Quartz::ConfigV1::Column_Pixels/4; break;
    default: break;
    }
    switch(c.vertical_binning()) {
    case Pds::Quartz::ConfigV1::x1: _defRows = Pds::Quartz::ConfigV1::Row_Pixels  ; break;
    case Pds::Quartz::ConfigV1::x2: _defRows = Pds::Quartz::ConfigV1::Row_Pixels/2; break;
    case Pds::Quartz::ConfigV1::x4: _defRows = Pds::Quartz::ConfigV1::Row_Pixels/4; break;
    default: break;
    }
  }
  else {
    FrameHandler::_configure(tid,payload,t);
  }
}
