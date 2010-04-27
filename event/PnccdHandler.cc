#include "PnccdHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/pnCCD/FrameV1.hh"

#include <string.h>
#include <stdio.h>

static const int PixelsPerBin = 2;

using namespace Ami;

static const unsigned rows = 1024;
static const unsigned cols = 1024;
static const unsigned rows_segment = 512;
static const unsigned cols_segment = 512;

PnccdHandler::PnccdHandler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_pnCCDframe, Pds::TypeId::Id_pnCCDconfig),
  _entry(0)
{
}

PnccdHandler::PnccdHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_pnCCDframe, Pds::TypeId::Id_pnCCDconfig),
  _entry(entry ? new EntryImage(entry->desc()) : 0)
{
}

PnccdHandler::~PnccdHandler()
{
}

unsigned PnccdHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* PnccdHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void PnccdHandler::reset() { _entry = 0; }

void PnccdHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  _config = *reinterpret_cast<const Pds::PNCCD::ConfigV1*>(payload);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescImage desc(det, 0, ChannelID::name(det),
		 cols / PixelsPerBin,
		 rows / PixelsPerBin,
		 PixelsPerBin, PixelsPerBin);
  _entry = new EntryImage(desc);
}

void PnccdHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}

void PnccdHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  const Pds::PNCCD::FrameV1* f = reinterpret_cast<const Pds::PNCCD::FrameV1*>(payload);
  if (!_entry) return;

  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));

  _fillQuadrant (f->data(), 0, 0);   
  f = f->next(_config);
  _fillQuadrant (f->data(), cols_segment, 0);
  f = f->next(_config);
  _fillQuadrantR(f->data(), cols_segment-1, rows-1);
  f = f->next(_config);
  _fillQuadrantR(f->data(), cols-1, rows-1);
  f = f->next(_config);

  //  _entry->info(f.offset()*PixelsPerBin*PixelsPerBin,EntryImage::Pedestal);
  _entry->info(0,EntryImage::Pedestal);
  _entry->info(1,EntryImage::Normalization);
  _entry->valid(t);
}

void PnccdHandler::_damaged() { _entry->invalid(); }

void PnccdHandler::_fillQuadrant(const uint16_t* d, unsigned x, unsigned y)
{
  //  PixelsPerBin = 2
  unsigned iy = y>>1;
  for(unsigned j=0; j<rows_segment; j+=2,iy++,d+=cols_segment) {
    const uint16_t* d1 = d+cols_segment;
    unsigned ix = x>>1;
    for(unsigned k=0; k<cols_segment; k+=2,ix++) {
      unsigned v = 
	(d [0]&0x3fff) + 
	(d [1]&0x3fff) + 
	(d1[0]&0x3fff) +
	(d1[1]&0x3fff);
      _entry->content(v, ix, iy);
      d  += 2;
      d1 += 2;
    }
  }
}

void PnccdHandler::_fillQuadrantR(const uint16_t* d, unsigned x, unsigned y)
{
  //  PixelsPerBin = 2
  unsigned iy = y>>1;
  for(unsigned j=0; j<rows_segment; j+=2,iy--,d+=cols_segment) {
    const uint16_t* d1 = d+cols_segment;
    unsigned ix = x>>1;
    for(unsigned k=0; k<cols_segment; k+=2,ix--) {
      unsigned v = 
	(d [0]&0x3fff) + 
	(d [1]&0x3fff) + 
	(d1[0]&0x3fff) +
	(d1[1]&0x3fff);
      _entry->content(v, ix, iy);
      d  += 2;
      d1 += 2;
    }
  }
}
