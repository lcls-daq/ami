#include "ami/event/Opal1kHandler.hh"
#include "pdsdata/xtc/DetInfo.hh"

static unsigned max_row_pixels   (const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::Opal1000: return 1024;
  case Pds::DetInfo::Opal1600: return 1200;
  case Pds::DetInfo::Opal2000: return 1080;
  case Pds::DetInfo::Opal4000: return 1752;
  case Pds::DetInfo::Opal8000: return 2472;
  default:       return 0;
  }
}

static unsigned max_column_pixels(const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::Opal1000: return 1024;
  case Pds::DetInfo::Opal1600: return 1600;
  case Pds::DetInfo::Opal2000: return 1920;
  case Pds::DetInfo::Opal4000: return 2336;
  case Pds::DetInfo::Opal8000: return 3296;
  default:       return 0;
  }
}

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
  unsigned n = max_column_pixels(info);
  return n;
}

static unsigned rows(const Pds::DetInfo& info) 
{
  unsigned n = max_row_pixels(info);
  return n;
}

Opal1kHandler::Opal1kHandler(const Pds::DetInfo& info) : 
  FrameHandler(info, 
               config_type_list(),
               columns(info),
               rows   (info))
{
}
