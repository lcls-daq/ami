#include "ami/event/Opal1kHandler.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/opal1k.ddl.h"

using namespace Pds;

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

void Opal1kHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_Opal1kConfig) {
    unsigned columns = max_column_pixels(static_cast<const DetInfo&>(info()));
    unsigned rows    = max_row_pixels   (static_cast<const DetInfo&>(info()));
    switch(tid.version()) {
    case 1:
      { const Pds::Opal1k::ConfigV1& c = *reinterpret_cast<const Pds::Opal1k::ConfigV1*>(payload);
        switch(c.vertical_binning()) {
        case Pds::Opal1k::ConfigV1::x2: rows/=2; break;
        case Pds::Opal1k::ConfigV1::x4: rows/=4; break;
        case Pds::Opal1k::ConfigV1::x8: rows/=8; break;
        default: break;
        }
      } break;
    }
    _defColumns = columns;
    _defRows    = rows;
  }
  else {
    FrameHandler::_configure(tid,payload,t);
  }
}
