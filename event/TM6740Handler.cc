#include "TM6740Handler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/camera/FrameV1.hh"
#include "pdsdata/camera/FrameFexConfigV1.hh"
#include "pdsdata/opal1k/ConfigV1.hh"

#include <string.h>

using namespace Ami;

//  Rotate the image clockwise or counter-clockwise
#define CLKWISE
//#define OPTROT

template <class T>
void _rfill(const Pds::Camera::FrameV1& f, EntryImage& entry)
{
  const DescImage& desc = entry.desc();

#ifndef OPTROT
  //
  //  Functional, but poor use of L1 cache
  //
  const T* d = reinterpret_cast<const T*>(f.data());
  for(unsigned j=0; j<f.height(); j++) {
#ifdef CLKWISE
    unsigned ix = desc.nbinsx()-1 - (desc.ppxbin()==2 ? j>>1 : j);
    if (desc.ppybin()==2)
      for(unsigned k=0; k<f.width(); k++, d++)
	entry.addcontent(*d, ix, k>>1);
    else
      for(unsigned k=0; k<f.width(); k++, d++)
	entry.addcontent(*d, ix, k);
#else
    unsigned ix = (desc.ppxbin()==2 ? j>>1 : j);
    if (desc.ppybin()==2)
      for(unsigned k=0; k<f.width(); k++, d++)
	entry.addcontent(*d, ix, desc.nbinsy()-1-(k>>1));
    else
      for(unsigned k=0; k<f.width(); k++, d++)
	entry.addcontent(*d, ix, desc.nbinsy()-1-k);
#endif
  }
#else
  //
  //  Rotate blocks with dimensions ~ cache line size
  //
  const unsigned block_size = 16;
  for(unsigned j=0; j<f.height(); j+=block_size) {
    for(unsigned k=0; k<f.width(); k+=block_size) {
      for(unsigned ij=j; ij<j+block_size; ij++) {
	const T* d = reinterpret_cast<const T*>(f.data()) + ij*desc.nbinsy()*desc.ppybin() + k;
#ifdef CLKWISE
	unsigned ix = desc.nbinsx()-1 - (desc.ppxbin()==2 ? ij>>1 : ij);
	if (desc.ppybin()==2)
	  for(unsigned ik=k; ik<k+block_size; ik++,d++)
	    entry.addcontent(*d, ix, ik>>1);
	else
	  for(unsigned ik=k; ik<k+block_size; ik++,d++)
	    entry.addcontent(*d, ix, ik);
#else
	unsigned ix = (desc.ppxbin()==2 ? ij>>1 : ij);
	if (desc.ppybin()==2)
	  for(unsigned ik=k; ik<k+block_size; ik++,d++)
	    entry.addcontent(*d, ix, desc.nbinsy()-1-(ik>>1));
	else
	  for(unsigned ik=k; ik<k+block_size; ik++,d++)
	    entry.addcontent(*d, ix, desc.nbinsy()-1-ik);
#endif
      }
    }
  }
#endif

  entry.info(f.offset()*desc.ppxbin()*desc.ppybin(),EntryImage::Pedestal);
  entry.info(1,EntryImage::Normalization);
}

#undef CLKWISE
#undef OPTROT

void TM6740Handler::_configure(const void* payload, const Pds::ClockTime& t)
{
  const Pds::Camera::FrameFexConfigV1& c = *reinterpret_cast<const Pds::Camera::FrameFexConfigV1*>(payload);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  unsigned columns,rows;
  if (c.forwarding() == Pds::Camera::FrameFexConfigV1::FullFrame) {
    columns = Pds::Pulnix::TM6740ConfigV1::Column_Pixels;
    rows    = Pds::Pulnix::TM6740ConfigV1::Row_Pixels;
  }
  else {
    columns = c.roiEnd().column-c.roiBegin().column;
    rows    = c.roiEnd().row   -c.roiBegin().row   ;
  }
  unsigned pixels  = (columns > rows) ? columns : rows;
  unsigned ppb     = (pixels-1)/640 + 1;
  columns /= ppb;
  rows    /= ppb;
  DescImage desc(det, 0, ChannelID::name(det),
		 //		 columns, rows, ppb, ppb);
		 rows, columns, ppb, ppb);

  if (_entry) 
    delete _entry;
  _entry = new EntryImage(desc);
  _entry->invalid();
}

void TM6740Handler::_event    (const void* payload, const Pds::ClockTime& t)
{
  const Pds::Camera::FrameV1& f = *reinterpret_cast<const Pds::Camera::FrameV1*>(payload);
  if (!_entry) return;

  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));

  if (f.depth_bytes()==2)
    _rfill<uint16_t>(f,*_entry);
  else
    _rfill<uint8_t >(f,*_entry);

  _entry->valid(t);
}

