#include "ami/event/Fccd960Handler.hh"
#include "pdsdata/xtc/DetInfo.hh"

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_PimImageConfig);
  types.push_back(Pds::TypeId::Id_FccdConfig);
  return types;
}

static unsigned columns(const Pds::DetInfo& info) 
{
  return (960u);
}

static unsigned rows(const Pds::DetInfo& info) 
{
  return (960u);
}

Fccd960Handler::Fccd960Handler(const Pds::DetInfo& info) : 
  FrameHandler(info, 
               config_type_list(),
               columns(info),
               rows   (info))
{
}
