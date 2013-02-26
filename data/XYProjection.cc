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

XYProjection::XYProjection(const DescEntry& output, 
			   Axis axis, double lo, double hi) :
  AbsOperator(AbsOperator::XYProjection),
  _axis      (axis),
  _lo        (lo),
  _hi        (hi),
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

DescEntry& XYProjection::_routput   () const 
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
  if (!e.valid())
    return *_output;
  
#define SET_BOUNDS_X(T)                                \
  T ilo = T((d.xlow()-inputd.xlow())/inputd.ppxbin()); \
  T ihi = T((d.xup ()-inputd.xlow())/inputd.ppxbin()); \
  T jlo = T((_lo-inputd.ylow())/inputd.ppybin());      \
  T jhi = T((_hi-inputd.ylow())/inputd.ppybin());

#define SET_BOUNDS_Y(T)                                 \
  T ilo = T((d.xlow()-inputd.ylow())/inputd.ppybin());  \
  T ihi = T((d.xup ()-inputd.ylow())/inputd.ppybin());  \
  T jlo = T((_lo-inputd.xlow())/inputd.ppxbin());       \
  T jhi = T((_hi-inputd.xlow())/inputd.ppxbin());

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
            for(unsigned i=ilo; i<=ihi; i++) {
              if (!mask->col(i)) continue;
              unsigned k=d.bin(inputd.xlow()+i*inputd.ppxbin());
              for(unsigned j=jlo; j<=jhi; j++) {
                if (mask->rowcol(j,i)) 
                  o->addcontent(double(_input->content(i,j))-p,k);
              }
            }
          }
	  else if (inputd.nframes()) {
            o->reset();
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
              SET_BOUNDS_X(int) ;
	      if (inputd.xy_bounds(ilo,ihi,jlo,jhi,fn))
		for(int i=ilo; i<=ihi; i++) {
		  unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
		  for(int j=jlo; j<=jhi; j++)
		    o->addcontent(double(_input->content(i,j))-p,k);
		}
	    }
	  }
          else {
            SET_BOUNDS_X(unsigned) ;
            for(unsigned i=ilo; i<=ihi; i++) {
              unsigned k=d.bin(inputd.xlow()+i*inputd.ppxbin());
              unsigned z=0;
              for(unsigned j=jlo; j<=jhi; j++)
                z += _input->content(i,j);
              o->content(double(z)-p*double(jhi-jlo),k);
            }
	  }
	}
	else {  // _axis == Y
          if (mask) {
            o->reset();
            SET_BOUNDS_Y(unsigned) ;
            for(unsigned i=ilo; i<=ihi; i++) {
              if (!mask->row(i)) continue;
              unsigned k=d.bin(inputd.ylow()+i*inputd.ppybin());
              for(unsigned j=jlo; j<=jhi; j++)
                if (mask->rowcol(i,j))
                  o->addcontent(_input->content(j,i)-p,k);
            }
          }
	  else if (inputd.nframes()) {
            o->reset();
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
              SET_BOUNDS_Y(int) ;
	      if (inputd.xy_bounds(jlo,jhi,ilo,ihi,fn))
		for(int i=ilo; i<=ihi; i++) {
		  unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
		  for(int j=jlo; j<=jhi; j++)
		    o->addcontent(double(_input->content(j,i))-p,k);
		}
	    }
	  }
          else {
            SET_BOUNDS_Y(unsigned) ;
            for(unsigned i=ilo; i<=ihi; i++) {
              unsigned k=d.bin(inputd.ylow()+i*inputd.ppybin());
              unsigned z=0;
              for(unsigned j=jlo; j<=jhi; j++)
                z += _input->content(j,i);
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
        const ImageMask* mask = inputd.mask();
	o->reset();
	
	if (_axis == X) {
	  if (mask) {
            SET_BOUNDS_X(unsigned) ;
	    for(unsigned i=ilo; i<=ihi; i++) {
              if (!mask->col(i)) continue;
	      unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
	      for(unsigned j=jlo; j<=jhi; j++)
                if (mask->rowcol(j,i))
                  o->addy(double(_input->content(i,j))-p,k);
	    }
	  }
	  else if (inputd.nframes()) {
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
              SET_BOUNDS_X(int) ;
	      if (inputd.xy_bounds(ilo,ihi,jlo,jhi,fn))
		for(int i=ilo; i<=ihi; i++) {
		  unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
		  for(int j=jlo; j<=jhi; j++)
		    o->addy(double(_input->content(i,j))-p,k);
		}
	    }
	  }
	  else {
            SET_BOUNDS_X(unsigned) ;
	    for(unsigned i=ilo; i<=ihi; i++) {
	      unsigned k = d.bin(inputd.xlow()+i*inputd.ppxbin());
	      for(unsigned j=jlo; j<=jhi; j++)
		o->addy(double(_input->content(i,j))-p,k);
	    }
	  }
	}
	else {
          if (mask) {
            SET_BOUNDS_Y(unsigned) ;
	    for(unsigned i=ilo; i<=ihi; i++) {
              if (!mask->row(i)) continue;
	      unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
	      for(unsigned j=jlo; j<=jhi; j++)
                if (mask->rowcol(i,j))
                  o->addy(double(_input->content(j,i))-p,k);
	    }
          }
	  else if (inputd.nframes()) {
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
              SET_BOUNDS_Y(int) ;
	      if (inputd.xy_bounds(jlo,jhi,ilo,ihi,fn))
		for(int i=ilo; i<=ihi; i++) {
		  unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
		  for(int j=jlo; j<=jhi; j++)
		    o->addy(double(_input->content(j,i))-p,k);
		}
	    }
	  }
	  else {
            SET_BOUNDS_Y(unsigned) ;
	    for(unsigned i=ilo; i<=ihi; i++) {
	      unsigned k = d.bin(inputd.ylow()+i*inputd.ppybin());
	      for(unsigned j=jlo; j<=jhi; j++)
		o->addy(double(_input->content(j,i))-p,k);
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
