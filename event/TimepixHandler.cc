#include "TimepixHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/timepix/DataV1.hh"
#include "pdsdata/timepix/DataV2.hh"
#include "pdsdata/timepix/ConfigV1.hh"
#include "pdsdata/timepix/ConfigV2.hh"

#include <string.h>

using namespace Ami;

TimepixHandler::TimepixHandler(const Pds::DetInfo& info) :
  EventHandler(info, Pds::TypeId::Id_TimepixData, Pds::TypeId::Id_TimepixConfig),
  _entry(0),
  _defColumns(Pds::Timepix::DataV1::Width + TimepixHandler::HorizGapPixels),
  _defRows   (Pds::Timepix::DataV1::Height + TimepixHandler::VertGapPixels)
{
}

TimepixHandler::TimepixHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_TimepixData, Pds::TypeId::Id_TimepixConfig),
  _entry(entry ? new EntryImage(entry->desc()) : 0)
{
}

TimepixHandler::~TimepixHandler()
{
  if (_entry)
    delete _entry;
}

unsigned TimepixHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* TimepixHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void TimepixHandler::reset() { _entry = 0; }

void TimepixHandler::_configure(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.version() < 1 && type.version() > 2) {
    printf("TimepixHandler::_configure: version %d is not supported\n", type.version());
    return;
  }

  //  const Pds::Timepix::ConfigV1& c = *reinterpret_cast<const Pds::Timepix::ConfigV1*>(payload);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  unsigned columns,rows;
  columns = _defColumns;
  rows    = _defRows;

  //   Fix resolution to 1 pixel
  unsigned ppb = 1;

  DescImage desc(det, (unsigned)0, ChannelID::name(det),
		 columns, rows, ppb, ppb);

  if (_entry) 
    delete _entry;
  _entry = new EntryImage(desc);

  // add a SubFrame for each quadrant
  unsigned qh = Pds::Timepix::DataV1::Height / 2;
  unsigned qw = Pds::Timepix::DataV1::Width / 2;
  _entry->desc().add_frame(0,            0,         qw, qh);
  _entry->desc().add_frame(columns - qw, 0,         qw, qh);
  _entry->desc().add_frame(0,            rows - qh, qw, qh);
  _entry->desc().add_frame(columns - qw, rows - qh, qw, qh);

  _entry->invalid();
}

void TimepixHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) {}

template <class T>
void _fill(const Pds::Timepix::DataV2& f, EntryImage& entry)
{
  T destVal;
  unsigned destX, destY;
  unsigned srcWidth = f.width();
  unsigned destWidth = entry.desc().nbinsx();
  unsigned srcHeight = f.height();
  unsigned destHeight = entry.desc().nbinsy();
  const T* d = reinterpret_cast<const T*>(f.data());

  // Copy 4 equally sized quadrants to the corners of the (larger) destination array.
  // Display error pixels (those over MaxPixelValue) as 0.

  for (destY=0; destY < srcHeight/2; destY++) {
    for (destX=0; destX < srcWidth/2; destX++, d++) {
      destVal = (*d > Pds::Timepix::DataV2::MaxPixelValue) ? 0 : *d;
      entry.addcontent(destVal, destX, destY);
    }
    for (destX = destWidth - (srcWidth/2); destX < destWidth; destX++, d++) {
      destVal = (*d > Pds::Timepix::DataV2::MaxPixelValue) ? 0 : *d;
      entry.addcontent(destVal, destX, destY);
    }
  }
  for (destY = destHeight - (srcHeight/2); destY < destHeight; destY++) {
    for (destX=0; destX < srcWidth/2; destX++, d++) {
      destVal = (*d > Pds::Timepix::DataV2::MaxPixelValue) ? 0 : *d;
      entry.addcontent(destVal, destX, destY);
    }
    for (destX = destWidth - (srcWidth/2); destX < destWidth; destX++, d++) {
      destVal = (*d > Pds::Timepix::DataV2::MaxPixelValue) ? 0 : *d;
      entry.addcontent(destVal, destX, destY);
    }
  }

  entry.info(0,EntryImage::Pedestal);
  entry.info(1,EntryImage::Normalization);
}

void TimepixHandler::_event(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (!_entry) return;

  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));

  if (type.version() == 2) {
    const Pds::Timepix::DataV2& dataV2 = *reinterpret_cast<const Pds::Timepix::DataV2*>(payload);
    _fill<uint16_t>(dataV2, *_entry);
  } else if (type.version() == 1) {
    const Pds::Timepix::DataV1& dataV1 = *reinterpret_cast<const Pds::Timepix::DataV1*>(payload);
    char buf[sizeof(dataV1) + dataV1.data_size()];
    const Pds::Timepix::DataV2& dataV2 = *new (buf) Pds::Timepix::DataV2(dataV1);
    _fill<uint16_t>(dataV2, *_entry);
  } else {
    printf("TimepixHandler: type.version()=%d is not supported\n", type.version());
  }

  _entry->valid(t);
}

void TimepixHandler::_damaged() { _entry->invalid(); }
