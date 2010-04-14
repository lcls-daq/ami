#include "FrameHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/camera/FrameV1.hh"
#include "pdsdata/camera/FrameFexConfigV1.hh"
#include "pdsdata/opal1k/ConfigV1.hh"

#include <string.h>

static const int PixelsPerBin = 2;

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

FrameHandler::FrameHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_FrameFexConfig),
  _entry(entry ? new EntryImage(entry->desc()) : 0)
{
}

FrameHandler::~FrameHandler()
{
}

unsigned FrameHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* FrameHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void FrameHandler::reset() { _entry = 0; }

void FrameHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  const Pds::Camera::FrameFexConfigV1& c = *reinterpret_cast<const Pds::Camera::FrameFexConfigV1*>(payload);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  unsigned columns,rows,ppb=1;
  if (c.forwarding() == Pds::Camera::FrameFexConfigV1::FullFrame) {
    columns = _defColumns;
    rows    = _defRows;
  }
  else {
    columns = c.roiEnd().column-c.roiBegin().column;
    rows    = c.roiEnd().row   -c.roiBegin().row   ;
  }
  if (columns > 640 || rows > 640) {
    columns /= PixelsPerBin;
    rows    /= PixelsPerBin;
    ppb      = PixelsPerBin;
  }
  DescImage desc(det, 0, ChannelID::name(det),
		 columns, rows, ppb, ppb);
  _entry = new EntryImage(desc);
}

void FrameHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}

void FrameHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  const Pds::Camera::FrameV1& f = *reinterpret_cast<const Pds::Camera::FrameV1*>(payload);
  if (!_entry) return;

  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));
  if (f.depth_bytes()==2) {
    const uint16_t* d = reinterpret_cast<const uint16_t*>(f.data());
    for(unsigned j=0; j<f.height(); j++)
      for(unsigned k=0; k<f.width(); k++, d++)
	_entry->addcontent(*d, k/PixelsPerBin, j/PixelsPerBin);
  }
  else {
    const uint8_t* d = reinterpret_cast<const uint8_t*>(f.data());
    for(unsigned j=0; j<f.height(); j++)
      for(unsigned k=0; k<f.width(); k++, d++)
	_entry->addcontent(*d, k/PixelsPerBin, j/PixelsPerBin);
  }
  _entry->info(f.offset()*PixelsPerBin*PixelsPerBin,EntryImage::Pedestal);
  _entry->info(1,EntryImage::Normalization);
  _entry->valid(t);
}

void FrameHandler::_damaged() { _entry->invalid(); }
