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
			   Axis axis, unsigned ilo, unsigned ihi) :
  AbsOperator(AbsOperator::XYProjection),
  _axis      (axis),
  _ilo       (ilo),
  _ihi       (ihi),
  _output    (0)
{
  memcpy(_desc_buffer, &output, output.size());
}

XYProjection::XYProjection(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::XYProjection)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_axis      , sizeof(_axis));
  _extract(p, &_ilo       , sizeof(_ilo ));
  _extract(p, &_ihi       , sizeof(_ihi ));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);
  _output = EntryFactory::entry(o);
}

XYProjection::XYProjection(const char*& p) :
  AbsOperator(AbsOperator::XYProjection),
  _output    (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_axis      , sizeof(_axis));
  _extract(p, &_ilo       , sizeof(_ilo ));
  _extract(p, &_ihi       , sizeof(_ihi ));
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
  _insert(p, &_ilo       , sizeof(_ilo ));
  _insert(p, &_ihi       , sizeof(_ihi ));
  return p;
}

Entry&     XYProjection::_operate(const Entry& e) const
{
  const EntryImage* _input = static_cast<const EntryImage*>(&e);
  if (_input) {
    unsigned ihi = _ihi;
    unsigned ilo = _ilo;
    switch(output().type()) {
    case DescEntry::TH1F:  // unnormalized
      { const DescTH1F& d = static_cast<const DescTH1F&>(output());
	EntryTH1F*      o = static_cast<EntryTH1F*>(_output);
	unsigned i=unsigned(d.xlow());
	for(unsigned k=0; k<d.nbins(); k++,i++) {
	  if (_axis == X) {
	    unsigned z=0;
	    for(unsigned j=ilo; j<ihi; j++)
	      z += _input->content(i,j);
	    z -= _input->info(EntryImage::Pedestal)*(ihi-ilo);
	    o->content(double(z),k);
	  }
	  else { // (_axis == Y)
	    unsigned z=0;
	    for(unsigned j=ilo; j<ihi; j++)
	      z += _input->content(j,i);
	    z -= _input->info(EntryImage::Pedestal)*(ihi-ilo);
	    o->content(double(z),k);
	  }
	}
	o->info(_input->info(EntryImage::Normalization),EntryTH1F::Normalization);
	break; }
    case DescEntry::Prof:  // normalized
      { const DescProf& d = static_cast<const DescProf&>(output());
	EntryProf*      o = static_cast<EntryProf*>(_output);
	const double    p = _input->info(EntryImage::Pedestal);
	o->reset();
	unsigned i=unsigned(d.xlow());
	for(unsigned k=0; k<d.nbins(); k++,i++) {
	  if (_axis == X) {
	    for(unsigned j=ilo; j<ihi; j++)
	      o->addy(_input->content(i,j)-p,k);
	  }
	  else if (_axis == Y) {
	    for(unsigned j=ilo; j<ihi; j++)
	      o->addy(_input->content(j,i)-p,k);
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
