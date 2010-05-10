#include "XYProjection.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"

#include <stdio.h>

using namespace Ami;

XYProjection::XYProjection(const DescEntry& output, 
			   Axis axis, double lo, double hi) :
  AbsOperator(AbsOperator::XYProjection),
  _axis      (axis),
  _lo        (lo),
  _hi        (hi),
  _output    (0)
{
  memcpy(_desc_buffer, &output, output.size());
}

XYProjection::XYProjection(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::XYProjection)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_axis      , sizeof(_axis));
  _extract(p, &_lo        , sizeof(_lo ));
  _extract(p, &_hi        , sizeof(_hi ));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);
  _output = EntryFactory::entry(o);
}

XYProjection::XYProjection(const char*& p) :
  AbsOperator(AbsOperator::XYProjection),
  _output    (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_axis      , sizeof(_axis));
  _extract(p, &_lo        , sizeof(_lo ));
  _extract(p, &_hi        , sizeof(_hi ));
}

XYProjection::~XYProjection()
{
  if (_output) delete _output;
}

DescEntry& XYProjection::output   () const 
{ 
  return _output ? _output->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

void*      XYProjection::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_axis      , sizeof(_axis));
  _insert(p, &_lo        , sizeof(_lo ));
  _insert(p, &_hi        , sizeof(_hi ));
  return p;
}

Entry&     XYProjection::_operate(const Entry& e) const
{
  const EntryImage* _input = static_cast<const EntryImage*>(&e);
  const DescImage& inputd = _input->desc();
  if (_input) {
    switch(output().type()) {
    case DescEntry::TH1F:  // unnormalized
      { const DescTH1F& d = static_cast<const DescTH1F&>(output());
	EntryTH1F*      o = static_cast<EntryTH1F*>(_output);

	if (_axis == X) {
	  unsigned ilo = unsigned((d.xlow()-inputd.xlow())/inputd.ppxbin());
	  unsigned ihi = unsigned((d.xup ()-inputd.xlow())/inputd.ppxbin());
	  unsigned jlo = unsigned((_lo-inputd.ylow())/inputd.ppybin());
	  unsigned jhi = unsigned((_hi-inputd.ylow())/inputd.ppybin());
	  for(unsigned i=ilo; i<ihi; i++) {
	    unsigned k=d.bin(inputd.xlow()+i*inputd.ppxbin());
	    unsigned z=0;
	    for(unsigned j=jlo; j<jhi; j++)
	      z += _input->content(i,j);
	    o->content(double(z)-double(_input->info(EntryImage::Pedestal)*(jhi-jlo)),k);
		       
	  }
	}
	else {
	  unsigned ilo = unsigned((d.xlow()-inputd.ylow())/inputd.ppybin());
	  unsigned ihi = unsigned((d.xup ()-inputd.ylow())/inputd.ppybin());
	  unsigned jlo = unsigned((_lo-inputd.xlow())/inputd.ppxbin());
	  unsigned jhi = unsigned((_hi-inputd.xlow())/inputd.ppxbin());
	  for(unsigned i=ilo; i<ihi; i++) {
	    unsigned k=d.bin(inputd.ylow()+i*inputd.ppybin());
	    unsigned z=0;
	    for(unsigned j=jlo; j<jhi; j++)
	      z += _input->content(j,i);
	    o->content(double(z)-double(_input->info(EntryImage::Pedestal)*(jhi-jlo)),k);
	  }
	}
	o->info(_input->info(EntryImage::Normalization),EntryTH1F::Normalization);
	break; }
    case DescEntry::Prof:  // normalized
      { const DescProf& d = static_cast<const DescProf&>(output());
	EntryProf*      o = static_cast<EntryProf*>(_output);
	const double    p = _input->info(EntryImage::Pedestal);
	o->reset();
	
	if (_axis == X) {
	  unsigned ilo = unsigned((d.xlow()-inputd.xlow())/inputd.ppxbin());
	  unsigned ihi = unsigned((d.xup ()-inputd.xlow())/inputd.ppxbin());
	  unsigned jlo = unsigned((_lo-inputd.ylow())/inputd.ppybin());
	  unsigned jhi = unsigned((_hi-inputd.ylow())/inputd.ppybin());
	  for(unsigned i=ilo; i<ihi; i++) {
	    unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
	    for(unsigned j=jlo; j<jhi; j++)
	      o->addy(double(_input->content(i,j))-p,k);
	  }
	}
	else {
	  unsigned ilo = unsigned((d.xlow()-inputd.ylow())/inputd.ppybin());
	  unsigned ihi = unsigned((d.xup ()-inputd.ylow())/inputd.ppybin());
	  unsigned jlo = unsigned((_lo-inputd.xlow())/inputd.ppxbin());
	  unsigned jhi = unsigned((_hi-inputd.xlow())/inputd.ppxbin());
	  for(unsigned i=ilo; i<ihi; i++) {
	    unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
	    for(unsigned j=jlo; j<jhi; j++)
	      o->addy(double(_input->content(j,i))-p,k);
	  }
	}

	o->info(_input->info(EntryImage::Normalization),EntryProf::Normalization);
	break; }
    default:
      break;
    }
    _output->valid(e.time());
  }
  return *_output;
}
