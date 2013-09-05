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
