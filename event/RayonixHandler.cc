#include "ami/event/RayonixHandler.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/rayonix.ddl.h"

static unsigned max_row_pixels   (const DetInfo& info)
{
  unsigned rv = 0;
  switch(info.device()) {
  case Pds::DetInfo::Rayonix: rv = 1920; break;
  default: break;
  }
  printf("%s returning %d\n", __FUNCTION__, rv);
  return (rv);
}

static unsigned max_column_pixels(const DetInfo& info)
{
  unsigned rv = 0;
  switch(info.device()) {
  case Pds::DetInfo::Rayonix: rv = 1920; break;
  default: break;
  }
  printf("%s returning %d\n", __FUNCTION__, rv);
  return (rv);
}

using namespace Ami;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  // uncomment the following line for FrameFexConfig support
  // types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_RayonixConfig);
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

RayonixHandler::RayonixHandler(const Pds::DetInfo& info) : 
  FrameHandler(info, 
               config_type_list(),
               columns(info),
               rows   (info))
{
}

void RayonixHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_RayonixConfig) {
    const Pds::Rayonix::ConfigV1* c = reinterpret_cast<const Pds::Rayonix::ConfigV1*>(payload);
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    // in case binning is 0, avoid divide-by-zero and use default value of 2
    int bin_f = c->binning_f() > 0 ? c->binning_f() : 2;
    int bin_s = c->binning_s() > 0 ? c->binning_s() : 2;
    unsigned columns = n_pixels_fast / bin_f;
    unsigned rows = n_pixels_slow / bin_s;
    unsigned ppb = image_ppbin(columns,rows,0);
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, rows, ppb, ppb);
    _entry = new EntryImage(desc);
    _entry->invalid();
  } else {
    printf("%s line %d: unexpected type ID (%u)\n", __FILE__, __LINE__, (unsigned)type);
  }
}
