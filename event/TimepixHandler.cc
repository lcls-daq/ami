#include "TimepixHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/timepix/DataV1.hh"
#include "pdsdata/timepix/ConfigV1.hh"

#include <string.h>

using namespace Ami;

TimepixHandler::TimepixHandler(const Pds::DetInfo& info) :
  EventHandler(info, Pds::TypeId::Id_TimepixData, Pds::TypeId::Id_TimepixConfig),
  _entry(0),
  _defColumns(TIMEPIX_WIDTH),
  _defRows   (TIMEPIX_HEIGHT)
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

void TimepixHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  //  const Pds::Timepix::ConfigV1& c = *reinterpret_cast<const Pds::Timepix::ConfigV1*>(payload);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  unsigned columns,rows;
  columns = _defColumns;
  rows    = _defRows;
  unsigned pixels  = (columns > rows) ? columns : rows;
  unsigned ppb     = (pixels-1)/640 + 1;

  //   Fix resolution to 1 pixel
  ppb = 1;

  columns = (columns+ppb-1)/ppb;
  rows    = (rows   +ppb-1)/ppb;
  DescImage desc(det, (unsigned)0, ChannelID::name(det),
		 columns, rows, ppb, ppb);

  if (_entry) 
    delete _entry;
  _entry = new EntryImage(desc);
  _entry->invalid();
}

void TimepixHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}

template <class T>
void _fill(const Pds::Timepix::DataV1& f, EntryImage& entry)
{
  const DescImage& desc = entry.desc();

  const T* d = reinterpret_cast<const T*>(f.data());
  for(unsigned j=0; j<f.height(); j++) {
    unsigned iy = desc.ppybin()==2 ? j>>1 : j;
    if (desc.ppxbin()==2)
      for(unsigned k=0; k<f.width(); k++, d++)
	entry.addcontent(*d, k>>1, iy);
    else
      for(unsigned k=0; k<f.width(); k++, d++)
	entry.addcontent(*d, k, iy);
  }

  //  entry.info(f.offset()*desc.ppxbin()*desc.ppybin(),EntryImage::Pedestal);
  entry.info(0,EntryImage::Pedestal);
  entry.info(1,EntryImage::Normalization);
}

void TimepixHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  const Pds::Timepix::DataV1& f = *reinterpret_cast<const Pds::Timepix::DataV1*>(payload);
  if (!_entry) return;

  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));

  if (f.depth_bytes()==2)
    _fill<uint16_t>(f,*_entry);
  else
    _fill<uint8_t >(f,*_entry);

  _entry->valid(t);
}

void TimepixHandler::_damaged() { _entry->invalid(); }