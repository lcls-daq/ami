#include "TM6740Handler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/camera/FrameV1.hh"
#include "pdsdata/pulnix/TM6740ConfigV1.hh"

#include <string.h>

static const int PixelsPerBin = 2;

using namespace Ami;

TM6740Handler::TM6740Handler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_TM6740Config),
  _entry(0)
{
}

TM6740Handler::TM6740Handler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_TM6740Config),
  _entry(entry ? new EntryImage(entry->desc()) : 0)
{
}

TM6740Handler::~TM6740Handler()
{
}

unsigned TM6740Handler::nentries() const { return _entry ? 1 : 0; }

const Entry* TM6740Handler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void TM6740Handler::reset() { _entry = 0; }

void TM6740Handler::_configure(const void* payload, const Pds::ClockTime& t)
{
  //  const Pds::Pulnix::TM6740ConfigV1& c = *reinterpret_cast<const Pds::Pulnix::TM6740ConfigV1*>(payload);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescImage desc(det, 0, ChannelID::name(det),
		 Pds::Pulnix::TM6740ConfigV1::Column_Pixels / PixelsPerBin,
		 Pds::Pulnix::TM6740ConfigV1::Row_Pixels / PixelsPerBin,
		 PixelsPerBin, PixelsPerBin);
  _entry = new EntryImage(desc);
}

void TM6740Handler::_calibrate(const void* payload, const Pds::ClockTime& t) {}

void TM6740Handler::_event    (const void* payload, const Pds::ClockTime& t)
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

void TM6740Handler::_damaged() { _entry->invalid(); }
