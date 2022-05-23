#include "ami/event/ArchonHandler.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/archon.ddl.h"

#include <stdio.h>

using namespace Pds;

static unsigned default_columns(const DetInfo& info)
{
  unsigned rv = 0;
  switch(info.device()) {
  case Pds::DetInfo::Archon: rv = 4800; break;
  default: break;
  }
  return rv;
}

static unsigned default_rows(const DetInfo& info)
{
  unsigned rv = 0;
  switch(info.device()) {
  case Pds::DetInfo::Archon: rv = 300; break;
  default: break;
  }
  return rv;
}

static inline unsigned num_rows(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                             \
  { const Pds::Archon::ConfigV##v& c =                                  \
      *reinterpret_cast<const Pds::Archon::ConfigV##v*>(tc->payload()); \
      return c.lines(); }

  switch(tc->contains.version()) {
    CASE_VSN(1)
    CASE_VSN(2);
    CASE_VSN(3);
    CASE_VSN(4);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned num_columns(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                             \
  { const Pds::Archon::ConfigV##v& c =                                  \
      *reinterpret_cast<const Pds::Archon::ConfigV##v*>(tc->payload()); \
      return c.pixels() * c.sensorTaps(); }

  switch(tc->contains.version()) {
    case 1:
      { const Pds::Archon::ConfigV1& c = 
          *reinterpret_cast<const Pds::Archon::ConfigV1*>(tc->payload());
          return c.pixels() * 8; }
    CASE_VSN(2);
    CASE_VSN(3);
    CASE_VSN(4);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_ArchonConfig);
  return types;
}

using namespace Ami;

ArchonHandler::ArchonHandler(const Pds::DetInfo& info) :
  FrameHandler(info,
               config_type_list(),
               default_columns(info),
               default_rows   (info))
{
}

void ArchonHandler::_configure(Pds::TypeId tid,
           const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_ArchonConfig) {
    const Xtc* tc = reinterpret_cast<const Xtc*>(payload)-1;
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    unsigned columns = num_columns(tc);
    unsigned rows = num_rows(tc);
    unsigned ppb = image_ppbin(columns,rows,0);
    _defColumns = columns;
    _defRows    = rows;
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
       columns, rows, ppb, ppb);
    _entry = new EntryImage(desc);
    _entry->invalid();
    _load_pedestals();
  } else {
    printf("%s line %d: unexpected type ID (%u)\n", __FILE__, __LINE__, (unsigned)type);
  }
}
