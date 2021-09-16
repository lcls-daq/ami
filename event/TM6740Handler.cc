#include "TM6740Handler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/pulnix.ddl.h"

#include <string.h>
#include <stdio.h>

using namespace Ami;

//  Rotate the image clockwise or counter-clockwise
#define CLKWISE
//#define OPTROT


static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_PimImageConfig);
  types.push_back(Pds::TypeId::Id_TM6740Config);
  return types;
}


template <class T>
void _rfill(const Pds::Camera::FrameV1& f, 
            const ndarray<int,2>&       p,
            EntryImage&                 entry)
{
  const DescImage& desc = entry.desc();

  const T* d = reinterpret_cast<const T*>(&f+1);

  ndarray<T,2> fc;
  if (p.size()) {
    fc = make_ndarray<T>(p.shape()[0],
                         p.shape()[1]);
    for(unsigned i=0; i<fc.shape()[0]; i++) {
      T*         pc = &fc(i,0);
      const int* pp = &p(i,0);
      for(unsigned j=0; j<fc.shape()[1]; j++)
        pc[j] = *d++ - pp[j];
    }
    d = fc.data();
  }

#ifndef OPTROT
  //
  //  Functional, but poor use of L1 cache
  //
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
	const T* d = reinterpret_cast<const T*>(&f+1) + ij*desc.nbinsy()*desc.ppybin() + k;
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

TM6740Handler::TM6740Handler(const Pds::DetInfo& info) :
  FrameHandler(info,
	       config_type_list(),
	       Pds::Pulnix::TM6740ConfigV1::Column_Pixels,
	       Pds::Pulnix::TM6740ConfigV1::Row_Pixels) ,
  _scale(1,1)
{
}

void TM6740Handler::_calibrate(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t) {}

void TM6740Handler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  Pds::TypeId::Type type = tid.id();
  if (type == Pds::TypeId::Id_FrameFexConfig) {
    if (_entry) 
      delete _entry;

    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
    unsigned columns = Pds::Pulnix::TM6740ConfigV1::Column_Pixels;
    unsigned rows    = Pds::Pulnix::TM6740ConfigV1::Row_Pixels;
    if (info().level()==Pds::Level::Source) {
      const Pds::Camera::FrameFexConfigV1& c = *reinterpret_cast<const Pds::Camera::FrameFexConfigV1*>(payload);
      if (c.forwarding() == Pds::Camera::FrameFexConfigV1::NoFrame)
	return;
      if (c.forwarding() != Pds::Camera::FrameFexConfigV1::FullFrame) {
        columns = c.roiEnd().column()-c.roiBegin().column();
        rows    = c.roiEnd().row   ()-c.roiBegin().row   ();
      }
    }
    unsigned ppb     = image_ppbin(columns, rows);

    DescImage desc(det, (unsigned)0, ChannelID::name(det),
		   //		 columns, rows, ppb, ppb);
		   rows, columns, ppb, ppb); // rotated size
    desc.set_scale(_scale.xscale(),_scale.yscale());

    _entry = new EntryImage(desc);
    _entry->invalid();
  }
  else if (type == Pds::TypeId::Id_PimImageConfig) {
    if (info().level()==Pds::Level::Source) {
      const Pds::Lusi::PimImageConfigV1& c = 
        *reinterpret_cast<const Pds::Lusi::PimImageConfigV1*>(payload);
      if (_entry) {
        _entry->desc().set_scale(c.xscale(),c.yscale());
      }
      else {
        _scale = c;
      }
    }
  }
}

void TM6740Handler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::Camera::FrameV1& f = *reinterpret_cast<const Pds::Camera::FrameV1*>(payload);

  memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));

  if (f.depth()>8)
    _rfill<uint16_t>(f,_pedestals,*_entry);
  else
    _rfill<uint8_t >(f,_pedestals,*_entry);

  _entry->valid(t);
}

