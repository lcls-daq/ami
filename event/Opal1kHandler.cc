#include "ami/event/Opal1kHandler.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/PimImageConfigType.hh"

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_PimImageConfig);
  types.push_back(Pds::TypeId::Id_Opal1kConfig);
  return types;
}

static unsigned columns(const Pds::DetInfo& info) 
{
  unsigned n = Opal1kConfigType::max_column_pixels(info);
  if (!n) n = Opal1kConfigType::Column_Pixels;
  return n;
}

static unsigned rows(const Pds::DetInfo& info) 
{
  unsigned n = Opal1kConfigType::max_row_pixels(info);
  if (!n) n = Opal1kConfigType::Row_Pixels;
  return n;
}

Opal1kHandler::Opal1kHandler(const Pds::DetInfo& info) : 
  FrameHandler(info, 
               config_type_list(),
               columns(info),
               rows   (info))
{
}
