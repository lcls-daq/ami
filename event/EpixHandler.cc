#include "EpixHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <string.h>

using namespace Ami;

EpixHandler::EpixHandler(const Pds::Src& info) :
  EventHandler(info, Pds::TypeId::Any, Pds::TypeId::Any),
  _entry (0),
  _config(0,0,0,0,0)
{
}

EpixHandler::~EpixHandler()
{
  if (_entry)
    delete _entry;
}

unsigned EpixHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* EpixHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void EpixHandler::reset() { _entry = 0; }

void EpixHandler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  if (tid.id()      == (Pds::TypeId::Type)Ami::Epix::ConfigT::typeId && 
      tid.version() == Ami::Epix::ConfigT::version) {
    if (_entry) 
      delete _entry;

    const Ami::Epix::ConfigT& c = *reinterpret_cast<const Ami::Epix::ConfigT*>(payload);
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    const unsigned chip_margin=4;
    unsigned columns = c.nchip_columns*c.ncolumns + (c.nchip_columns-1)*chip_margin;
    unsigned rows    = c.nchip_rows   *c.nrows    + (c.nchip_rows   -1)*chip_margin;
    unsigned pixels  = (columns > rows) ? columns : rows;
    unsigned ppb     = _full_resolution() ? 1 : (pixels-1)/640 + 1;
    columns = (columns+ppb-1)/ppb;
    rows    = (rows   +ppb-1)/ppb;

    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   columns, rows, ppb, ppb);
    for(unsigned i=0; i<c.nchip_rows; i++)
      for(unsigned j=0; j<c.nchip_columns; j++)
	desc.add_frame(j*(c.ncolumns+chip_margin)/ppb,
		       i*(c.nrows   +chip_margin)/ppb,
		       c.ncolumns/ppb,
		       c.nrows   /ppb);

    _entry = new EntryImage(desc);
    _entry->invalid();

    _config = c;
  }
}

void EpixHandler::_configure(const void* payload, const Pds::ClockTime& t) {}
void EpixHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}

#include "pdsdata/xtc/ClockTime.hh"

bool EpixHandler::used() const { return (_entry && _entry->desc().used()); }

void EpixHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  const Ami::Epix::DataT& f = *reinterpret_cast<const Ami::Epix::DataT*>(payload);

  _entry->reset();
  const DescImage& d = _entry->desc();

  const uint16_t* p = reinterpret_cast<const uint16_t*>(&f+1);

  for(unsigned i=0; i<_config.nrows; i++)
    for(unsigned j=0; j<_config.ncolumns; j++) {
      unsigned fn=0;
      for(unsigned k=0; k<_config.nchip_rows; k++)
	for(unsigned m=0; m<_config.nchip_columns; m++,fn++) {
	  const SubFrame& f = _entry->desc().frame(fn);
	  _entry->addcontent(p[fn], f.x+j/d.ppxbin(), f.y+i/d.ppybin());
	}
      p += ((fn+1)&~1)*_config.nsamples;
    }

  _entry->info(0.,EntryImage::Pedestal);
  _entry->info(1.,EntryImage::Normalization);
  _entry->valid(t);

}

void EpixHandler::_damaged() { _entry->invalid(); }
