#include "ami/event/QuartzHandler.hh"
#include "pds/config/QuartzConfigType.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/PimImageConfigType.hh"

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
               QuartzConfigType::max_column_pixels(info),
               QuartzConfigType::max_row_pixels(info)) 
{
}
