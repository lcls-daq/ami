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
    unsigned columns = Pds::Quartz::ConfigV1::Column_Pixels;
    unsigned rows    = Pds::Quartz::ConfigV1::Row_Pixels;
    switch(tid.version()) {
    case 1:
      { const Pds::Quartz::ConfigV1& c = *reinterpret_cast<const Pds::Quartz::ConfigV1*>(payload);
        switch(c.horizontal_binning()) {
        case Pds::Quartz::ConfigV1::x2: columns/=2; break;
        case Pds::Quartz::ConfigV1::x4: columns/=4; break;
        default: break;
        }
        switch(c.vertical_binning()) {
        case Pds::Quartz::ConfigV1::x2: rows/=2; break;
        case Pds::Quartz::ConfigV1::x4: rows/=4; break;
        default: break;
        }
      } break;
    case 2:
      { const Pds::Quartz::ConfigV2& c = *reinterpret_cast<const Pds::Quartz::ConfigV2*>(payload);
        if (c.use_hardware_roi()) {
          columns = c.roi_hi().column()-c.roi_lo().column()+1;
          rows    = c.roi_hi().row   ()-c.roi_lo().row   ()+1;
        }
        switch(c.horizontal_binning()) {
        case Pds::Quartz::ConfigV1::x2: columns/=2; break;
        case Pds::Quartz::ConfigV1::x4: columns/=4; break;
        default: break;
        }
        switch(c.vertical_binning()) {
        case Pds::Quartz::ConfigV1::x2: rows/=2; break;
        case Pds::Quartz::ConfigV1::x4: rows/=4; break;
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
