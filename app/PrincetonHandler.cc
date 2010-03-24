#include "PrincetonHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/camera/FrameV1.hh"
#include "pdsdata/princeton/ConfigV1.hh"

#include <string.h>

static const int PixelsPerBin = 2;

using namespace Ami;

PrincetonHandler::PrincetonHandler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_PrincetonConfig),
  _entry(0)
{
}

PrincetonHandler::PrincetonHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_PrincetonConfig),
  _entry(entry ? new EntryImage(entry->desc()) : 0)
{
}

PrincetonHandler::~PrincetonHandler()
{
}

unsigned PrincetonHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* PrincetonHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void PrincetonHandler::reset() { _entry = 0; }

void PrincetonHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  const Pds::Princeton::ConfigV1& c = *reinterpret_cast<const Pds::Princeton::ConfigV1*>(payload);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescImage desc(det, 0, ChannelID::name(det),
		 c.width()  / PixelsPerBin,
		 c.height() / PixelsPerBin,
		 PixelsPerBin, PixelsPerBin);
  _entry = new EntryImage(desc);
}

void PrincetonHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}

void PrincetonHandler::_event    (const void* payload, const Pds::ClockTime& t)
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

void PrincetonHandler::_damaged() { _entry->invalid(); }
