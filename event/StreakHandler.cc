#include "ami/event/StreakHandler.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/valgnd.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/streak.ddl.h"

using namespace Pds;

static const unsigned UNITS_LEN = 64;

static unsigned max_row_pixels   (const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::StreakC7700: return 1024;
  default:       return 0;
  }
}

static unsigned max_column_pixels(const DetInfo& info)
{
  switch(info.device()) {
  case Pds::DetInfo::StreakC7700: return 1344;
  default:       return 0;
  }
}

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_StreakConfig);
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

StreakHandler::StreakHandler(const Pds::DetInfo& info) : 
  FrameHandler(info, 
               config_type_list(),
               columns(info),
               rows   (info)),
  _scale(1.0)
{
  memset(_units, 0, UnitsSize);
}

void StreakHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_StreakConfig) {
    unsigned columns = max_column_pixels(static_cast<const DetInfo&>(info()));
    unsigned rows    = max_row_pixels   (static_cast<const DetInfo&>(info()));
    switch(tid.version()) {
    case 1:
      { const Pds::Streak::ConfigV1& c = *reinterpret_cast<const Pds::Streak::ConfigV1*>(payload);
        columns = Pds::Streak::ConfigV1::Column_Pixels;
        rows = Pds::Streak::ConfigV1::Row_Pixels;
        ndarray<const double, 1> calib = c.calib();
        double cal_sum = 0;
        double xy_sum = 0;
        double xx_sum = 0;
        for (unsigned n=1; n<columns; n++) {
            cal_sum += calib[0] + calib[1] * n + calib[2] * n * n;
            xy_sum  += n * cal_sum;
            xx_sum  += n * n;
        }
        _scale = xy_sum / xx_sum;
        switch (c.calibScale()) {
          case Pds::Streak::ConfigV1::Nanoseconds:
            set_units("ns");
            break;
          case Pds::Streak::ConfigV1::Microseconds:
            set_units("us");
            break;
          case Pds::Streak::ConfigV1::Milliseconds:
            set_units("ms");
            break;
          case Pds::Streak::ConfigV1::Seconds:
            set_units("s");
            break;
          default:
            set_units("");
            break;
        }
      } break;
    }
    _defColumns = columns;
    _defRows    = rows;
    if (_entry) {
      _entry->desc().set_scale(_scale,1.0);
      _entry->desc().set_units(_units);
    }
  }
  else {
    FrameHandler::_configure(tid,payload,t);
    if (type == Pds::TypeId::Id_FrameFexConfig && _entry) {
      _entry->desc().set_scale(_scale,1.0);
      _entry->desc().set_units(_units);
    }
  }
}

void StreakHandler::set_units(const char* units)
{
  if (units) {
    strncpy_val(_units, units, UnitsSize);
    _units[UnitsSize-1] = 0;
  }
}
