#include "ami/event/PhasicsHandler.hh"
#include "pds/config/PhasicsConfigType.hh"
#include "pdsdata/camera/FrameFexConfigV1.hh"

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_PhasicsConfig);
  return types;
}

static Pds::Camera::FrameCoord roi(0,0);

static Pds::Camera::FrameFexConfigV1 _dummy(Pds::Camera::FrameFexConfigV1::FullFrame,
    1, Pds::Camera::FrameFexConfigV1::NoProcessing,
    roi, roi, 0, 0, 0);

PhasicsHandler::PhasicsHandler(const Pds::DetInfo& info) :
  FrameHandler(info, 
               config_type_list(),
               PhasicsConfigType::Width,
               PhasicsConfigType::Height)
{
}

void PhasicsHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  FrameHandler::_configure(&_dummy,t);
}
