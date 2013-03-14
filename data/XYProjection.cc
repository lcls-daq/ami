#include "XYProjection.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/ImageMask.hh"

#include "ami/data/Cds.hh"

#include <stdio.h>

using namespace Ami;

XYProjection::XYProjection(const DescEntry& output, Axis axis) :
  AbsOperator(AbsOperator::XYProjection),
  _axis      (axis),
  _output    (0)
{
  memcpy(_desc_buffer, &output, output.size());
  memset(_desc_buffer+output.size(), 0, DESC_LEN-output.size());
}

XYProjection::XYProjection(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::XYProjection)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_axis      , sizeof(_axis));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);
  const DescImage& d =  reinterpret_cast<const DescImage&>(input);
  int nx;
  double x0,x1;
  if (_axis==X) {
    nx = d.nbinsx();
    x0 = d.xlow();
    x1 = d.xup ();
  }
  else {
    nx = d.nbinsy();
    x0 = d.ylow();
    x1 = d.yup ();
  }

  switch(o.type()) {
  case DescEntry::TH1F:
    _output = EntryFactory::entry(DescTH1F(o.name(),o.xtitle(),o.ytitle(),nx,x0,x1));
    break;
  case DescEntry::Prof:
    _output = EntryFactory::entry(DescProf(o.name(),o.xtitle(),o.ytitle(),nx,x0,x1,""));
    break;
  default:
    _output = 0;
    break;
  }
}

XYProjection::XYProjection(const char*& p) :
  AbsOperator(AbsOperator::XYProjection),
  _output    (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_axis      , sizeof(_axis));
}

XYProjection::~XYProjection()
{
  if (_output) delete _output;
}

DescEntry& XYProjection::_routput   () const 
{ 
  return _output ? _output->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

void*      XYProjection::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_axis      , sizeof(_axis));
  return p;
}

Entry&     XYProjection::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output;
  
#define SET_BOUNDS_X(T)                         \
  T ilo = 0;                                    \
  T ihi = inputd.nbinsx();                      \
  T jlo = 0;                                    \
  T jhi = inputd.nbinsy();
  
#define SET_BOUNDS_Y(T)                                 \
  T ilo = 0;                                            \
  T ihi = inputd.nbinsy();                              \
  T jlo = 0;                                            \
  T jhi = inputd.nbinsx(); 
  
  const EntryImage* _input = static_cast<const EntryImage*>(&e);
  const DescImage& inputd = _input->desc();
  if (_input) {
    switch(_routput().type()) {
    case DescEntry::TH1F:  // unnormalized
      { const DescTH1F& d = static_cast<const DescTH1F&>(_routput());
	EntryTH1F*      o = static_cast<EntryTH1F*>(_output);
	const double    p = _input->info(EntryImage::Pedestal);
        const ImageMask*   mask = inputd.mask();
	if (_axis == X) {
          if (mask) {
            o->reset();
            SET_BOUNDS_X(unsigned) ;
            for(unsigned i=ilo; i<ihi; i++) {
              if (!mask->col(i)) continue;
              double z = 0;
              for(unsigned j=jlo; j<jhi; j++) {
                if (mask->rowcol(j,i)) 
                  z += double(_input->content(i,j))-p;
              }
              unsigned k=d.bin(inputd.xlow()+i*inputd.ppxbin());
              o->addcontent(z,k);
            }
          }
	  else if (inputd.nframes()) {
            o->reset();
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
              SET_BOUNDS_X(int) ;
	      if (inputd.xy_bounds(ilo,ihi,jlo,jhi,fn))
		for(int i=ilo; i<=ihi; i++) {
                  unsigned z=0;
		  for(int j=jlo; j<=jhi; j++)
		    z += _input->content(i,j);
		  unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
                  o->addcontent(double(z)-double(jhi-jlo+1)*p,k);
		}
	    }
	  }
          else {
            SET_BOUNDS_X(unsigned) ;
            for(unsigned i=ilo; i<ihi; i++) {
              unsigned z=0;
              for(unsigned j=jlo; j<jhi; j++)
                z += _input->content(i,j);
              unsigned k=d.bin(inputd.xlow()+i*inputd.ppxbin());
              o->content(double(z)-p*double(jhi-jlo),k);
            }
	  }
	}
	else {  // _axis == Y
          if (mask) {
            o->reset();
            SET_BOUNDS_Y(unsigned) ;
            for(unsigned i=ilo; i<ihi; i++) {
              if (!mask->row(i)) continue;
              double z=0;
              for(unsigned j=jlo; j<jhi; j++)
                if (mask->rowcol(i,j))
                  z += double(_input->content(j,i))-p;
              unsigned k=d.bin(inputd.ylow()+i*inputd.ppybin());
              o->addcontent(z,k);
            }
          }
	  else if (inputd.nframes()) {
            o->reset();
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
              SET_BOUNDS_Y(int) ;
	      if (inputd.xy_bounds(jlo,jhi,ilo,ihi,fn))
		for(int i=ilo; i<=ihi; i++) {
                  unsigned z=0;
		  for(int j=jlo; j<=jhi; j++)
                    z += _input->content(j,i);
		  unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
                  o->addcontent(double(z)-double(jhi-jlo+1)*p,k);
		}
	    }
	  }
          else {
            SET_BOUNDS_Y(unsigned) ;
            for(unsigned i=ilo; i<ihi; i++) {
              unsigned z=0;
              for(unsigned j=jlo; j<jhi; j++)
                z += _input->content(j,i);
              unsigned k=d.bin(inputd.ylow()+i*inputd.ppybin());
              o->content(double(z)-p*double(jhi-jlo),k);
            }
	  }
	}
	o->info(_input->info(EntryImage::Normalization),EntryTH1F::Normalization);
	break; }
    case DescEntry::Prof:  // normalized
      { const DescProf& d = static_cast<const DescProf&>(_routput());
	EntryProf*      o = static_cast<EntryProf*>(_output);
	const double    p = _input->info(EntryImage::Pedestal);
        const double    q = double(inputd.ppxbin()*inputd.ppybin());
        const ImageMask* mask = inputd.mask();
	o->reset();
	
	if (_axis == X) {
	  if (mask) {
            SET_BOUNDS_X(unsigned) ;
	    for(unsigned i=ilo; i<ihi; i++) {
              if (!mask->col(i)) continue;
	      unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
	      for(unsigned j=jlo; j<jhi; j++)
                if (mask->rowcol(j,i))
                  o->addy((double(_input->content(i,j))-p)/q,k);
	    }
	  }
	  else if (inputd.nframes()) {
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
              SET_BOUNDS_X(int) ;
	      if (inputd.xy_bounds(ilo,ihi,jlo,jhi,fn))
		for(int i=ilo; i<=ihi; i++) {
		  unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
		  for(int j=jlo; j<=jhi; j++)
		    o->addy((double(_input->content(i,j))-p)/q,k);
		}
	    }
	  }
	  else {
            SET_BOUNDS_X(unsigned) ;
	    for(unsigned i=ilo; i<ihi; i++) {
	      unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
	      for(unsigned j=jlo; j<jhi; j++)
		o->addy((double(_input->content(i,j))-p)/q,k);
	    }
	  }
	}
	else {
          if (mask) {
            SET_BOUNDS_Y(unsigned) ;
	    for(unsigned i=ilo; i<ihi; i++) {
              if (!mask->row(i)) continue;
	      unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
	      for(unsigned j=jlo; j<jhi; j++)
                if (mask->rowcol(i,j))
                  o->addy((double(_input->content(j,i))-p)/q,k);
	    }
          }
	  else if (inputd.nframes()) {
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
              SET_BOUNDS_Y(int) ;
	      if (inputd.xy_bounds(jlo,jhi,ilo,ihi,fn))
		for(int i=ilo; i<=ihi; i++) {
		  unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
		  for(int j=jlo; j<=jhi; j++)
		    o->addy((double(_input->content(j,i))-p)/q,k);
		}
	    }
	  }
	  else {
            SET_BOUNDS_Y(unsigned) ;
	    for(unsigned i=ilo; i<ihi; i++) {
	      unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
	      for(unsigned j=jlo; j<jhi; j++)
		o->addy((double(_input->content(j,i))-p)/q,k);
	    }
	  }
	}
	o->info(_input->info(EntryImage::Normalization),EntryProf::Normalization);
	break; }
    default:
      break;
    }
  }

#undef SET_BOUNDS_X
#undef SET_BOUNDS_Y

  _output->valid(e.time());
  return *_output;
}
