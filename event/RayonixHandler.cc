#include "ami/event/RayonixHandler.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/rayonix.ddl.h"

#include <stdio.h>

using namespace Pds;

static unsigned max_row_pixels   (const DetInfo& info)
{
  unsigned rv = 0;
  switch(info.device()) {
  case Pds::DetInfo::Rayonix: rv = 3840; break;
  default: break;
  }
  printf("%s returning %d\n", __FUNCTION__, rv);
  return (rv);
}

static unsigned max_column_pixels(const DetInfo& info)
{
  unsigned rv = 0;
  switch(info.device()) {
  case Pds::DetInfo::Rayonix: rv = 3840; break;
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
#define CASE_VSN(v) case v:                                             \
  { const Pds::Rayonix::ConfigV##v* c =                                 \
      reinterpret_cast<const Pds::Rayonix::ConfigV##v*>(payload);       \
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info()); \
    int bin_f = c->binning_f() > 0 ? c->binning_f() : 2;                \
    int bin_s = c->binning_s() > 0 ? c->binning_s() : 2;                \
    unsigned columns = c->maxWidth() / bin_f;                           \
    unsigned rows = c->maxHeight() / bin_s;                             \
    unsigned ppb = image_ppbin(columns,rows,0);                         \
    DescImage desc(det, (unsigned)0, ChannelID::name(det),              \
                     columns, rows, ppb, ppb);                          \
    _entry = new EntryImage(desc);                                      \
    _entry->invalid(); } break;

  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_RayonixConfig) {
    switch(tid.version()) {
      CASE_VSN(1);
      CASE_VSN(2);
      default:
        printf("%s line %d: unexpected type version (%u)\n", __FILE__, __LINE__, (unsigned)tid.version());
        break;
    }
  } else {
    printf("%s line %d: unexpected type ID (%u)\n", __FILE__, __LINE__, (unsigned)type);
  }
#undef CASE_VSN
}
