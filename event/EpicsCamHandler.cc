#include "ami/event/EpicsCamHandler.hh"
#include "ami/data/EntryImage.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/camera.ddl.h"

using namespace Pds;

static unsigned max_row_pixels   (const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::ControlsCamera: return 1024;
  case Pds::DetInfo::TM6740:   return 480;
  case Pds::DetInfo::OrcaFl40: return 2048;
  case Pds::DetInfo::Rayonix:  return 1920;
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
  case Pds::DetInfo::ControlsCamera: return 1024;
  case Pds::DetInfo::TM6740:   return 640;
  case Pds::DetInfo::OrcaFl40: return 2048;
  case Pds::DetInfo::Rayonix:  return 1920;
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
  types.push_back(Pds::TypeId::Id_ControlsCameraConfig);
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

EpicsCamHandler::EpicsCamHandler(const Pds::DetInfo& info) : 
  FrameHandler(info, 
               config_type_list(),
               columns(info),
               rows   (info)),
  _scale(1,1)
{
}

void EpicsCamHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_ControlsCameraConfig) {
    unsigned columns = max_column_pixels(static_cast<const DetInfo&>(info()));
    unsigned rows    = max_row_pixels   (static_cast<const DetInfo&>(info()));
    switch(tid.version()) {
    case 1:
      { const Pds::Camera::ControlsCameraConfigV1& c = *reinterpret_cast<const Pds::Camera::ControlsCameraConfigV1*>(payload);
        columns = c.width();
        rows = c.height();
      } break;
    }
    _defColumns = columns;
    _defRows    = rows;
  }
  else if (type == Pds::TypeId::Id_PimImageConfig) {
    if (info().level()==Pds::Level::Source) {
      const Pds::Lusi::PimImageConfigV1& c =
        *reinterpret_cast<const Pds::Lusi::PimImageConfigV1*>(payload);
      if (_entry) {
        _entry->desc().set_scale(c.xscale(),c.yscale());
      }
      else {
        _scale = c;
      }
    }
  }
  else {
    FrameHandler::_configure(tid,payload,t);
    if (type == Pds::TypeId::Id_FrameFexConfig && _entry) {
      _entry->desc().set_scale(_scale.xscale(),_scale.yscale());
    }
  }
}
