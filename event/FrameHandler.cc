#include "FrameHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/opal1k.ddl.h"

#include <string.h>

using namespace Ami;

FrameHandler::FrameHandler(const Pds::DetInfo& info,
			   unsigned defColumns,
			   unsigned defRows) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_FrameFexConfig),
  _entry(0),
  _defColumns(defColumns),
  _defRows   (defRows)
{
}

FrameHandler::FrameHandler(const Pds::DetInfo& info,
			   const std::list<Pds::TypeId::Type>& config_types,
			   unsigned defColumns,
			   unsigned defRows) : 
  EventHandler(info, Pds::TypeId::Id_Frame, config_types),
  _entry(0),
  _defColumns(defColumns),
  _defRows   (defRows)
{
}

FrameHandler::FrameHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_FrameFexConfig),
  _entry(entry ? new EntryImage(entry->desc()) : 0)
{
}

FrameHandler::~FrameHandler()
{
  if (_entry)
    delete _entry;
}

unsigned FrameHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* FrameHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void FrameHandler::reset() { _entry = 0; }

void FrameHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_FrameFexConfig) {
    if (_entry) 
      delete _entry;

    const Pds::Camera::FrameFexConfigV1& c = *reinterpret_cast<const Pds::Camera::FrameFexConfigV1*>(payload);
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    unsigned columns,rows;
    switch(c.forwarding()) {
    case Pds::Camera::FrameFexConfigV1::FullFrame:
      columns = _defColumns;
      rows    = _defRows;
      break;
    case Pds::Camera::FrameFexConfigV1::RegionOfInterest:
      columns = c.roiEnd().column()-c.roiBegin().column();
      rows    = c.roiEnd().row   ()-c.roiBegin().row   ();
      break;
    case Pds::Camera::FrameFexConfigV1::NoFrame:
    default:
      return;
    }
    unsigned pixels  = (columns > rows) ? columns : rows;
    unsigned ppb     = _full_resolution() ? 1 : (pixels-1)/640 + 1;
    columns = (columns+ppb-1)/ppb;
    rows    = (rows   +ppb-1)/ppb;
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, rows, ppb, ppb);
    _entry = new EntryImage(desc);
    _entry->invalid();
  }
}

void FrameHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

#include "pdsdata/xtc/ClockTime.hh"

void FrameHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  if (!_entry || !_entry->desc().used()) return;

  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));

  const DescImage& desc = _entry->desc();

  const Pds::Camera::FrameV1& f = *reinterpret_cast<const Pds::Camera::FrameV1*>(payload);
  if (f.depth()>8)
    _entry->content(f.data16());
  else
    _entry->content(f.data8());

  _entry->info(f.offset()*desc.ppxbin()*desc.ppybin(),EntryImage::Pedestal);
  _entry->info(1,EntryImage::Normalization);
  _entry->valid(t);
}

void FrameHandler::_damaged() { _entry->invalid(); }
