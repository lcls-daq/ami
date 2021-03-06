#include "ContourProjection.hh"

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

ContourProjection::ContourProjection(const DescEntry& output, 
				     const Contour& contour,
				     Axis axis, 
				     double xlo, double xhi,
				     double ylo, double yhi) :
  AbsOperator(AbsOperator::ContourProjection),
  _contour   (contour),
  _axis      (axis),
  _xlo       (xlo),
  _xhi       (xhi),
  _ylo       (ylo),
  _yhi       (yhi),
  _output    (0),
  _offset_len(0),
  _offset    (0)
{
  memcpy_val(_desc_buffer, &output, output.size(), DESC_LEN);
}

ContourProjection::ContourProjection(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::ContourProjection)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_contour   , sizeof(_contour));
  _extract(p, &_axis      , sizeof(_axis));
  _extract(p, &_xlo       , sizeof(_xlo ));
  _extract(p, &_xhi       , sizeof(_xhi ));
  _extract(p, &_ylo       , sizeof(_ylo ));
  _extract(p, &_yhi       , sizeof(_yhi ));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);
  _output = EntryFactory::entry(o);

  _offset_len = 0;
  _offset     = 0;
}

ContourProjection::ContourProjection(const char*& p) :
  AbsOperator(AbsOperator::ContourProjection),
  _output    (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_contour   , sizeof(_contour));
  _extract(p, &_axis      , sizeof(_axis));
  _extract(p, &_xlo       , sizeof(_xlo ));
  _extract(p, &_xhi       , sizeof(_xhi ));
  _extract(p, &_ylo       , sizeof(_ylo ));
  _extract(p, &_yhi       , sizeof(_yhi ));

  _offset_len = 0;
  _offset     = 0;
}

ContourProjection::~ContourProjection()
{
  if (_output) delete _output;
  if (_offset) delete[] _offset;
}

const DescEntry& ContourProjection::_routput   () const 
{ 
  return _output ? _output->desc() : *reinterpret_cast<const DescEntry*>(_desc_buffer); 
}

void*      ContourProjection::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_contour   , sizeof(_contour));
  _insert(p, &_axis      , sizeof(_axis));
  _insert(p, &_xlo       , sizeof(_xlo ));
  _insert(p, &_xhi       , sizeof(_xhi ));
  _insert(p, &_ylo       , sizeof(_ylo ));
  _insert(p, &_yhi       , sizeof(_yhi ));
  return p;
}

Entry&     ContourProjection::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output;

  const EntryImage* _input = static_cast<const EntryImage*>(&e);
  double ped = (double)(_input->info(EntryImage::Pedestal));
  const DescImage& inputd  = _input->desc();
  const ImageMask* mask    = inputd.mask();
  ContourProjection* pthis = const_cast<ContourProjection*>(this);
  if (_input) {
    //
    //  Recompute the contour offsets when they change
    //
    if      (_axis==Y && _offset==0) {
      pthis->_offset_len = inputd.xbin(_xhi)-inputd.xbin(_xlo)+1;
      pthis->_offset = new int16_t[_offset_len];
      for(unsigned i=0,ix=inputd.xbin(_xlo); i<_offset_len; i++,ix++) {
	pthis->_offset[i] = int16_t(_contour.value(inputd.binx(ix)));
      }
    }
    else if (_axis==X && _offset==0) {
      pthis->_offset_len = inputd.ybin(_yhi)-inputd.ybin(_ylo)+1;
      pthis->_offset = new int16_t[_offset_len];
      for(unsigned i=0,iy=inputd.ybin(_ylo); i<_offset_len; i++,iy++)
	pthis->_offset[i] = int16_t(_contour.value(inputd.biny(iy)));
    }
    //
    //  Fill the projection
    //
    switch(_routput().type()) {
    case DescEntry::TH1F:  // unnormalized
      { const DescTH1F& d = static_cast<const DescTH1F&>(_routput());
	EntryTH1F*      o = static_cast<EntryTH1F*>(_output);
	o->clear();
	if (_axis == X) {
	  const double q  = double(inputd.ppxbin());
	  const double dl = _contour.discrimLevel()*double(inputd.ppybin());
          if (mask) {
            unsigned ixlo = inputd.xbin(_xlo);
            unsigned ixhi = inputd.xbin(_xhi);
            unsigned iylo = inputd.ybin(_ylo);
            unsigned iyhi = inputd.ybin(_yhi);
            const int16_t* off = _offset;
            for(unsigned j=iylo; j<=iyhi; j++,off++) {
              if (!mask->row(j)) continue;
              for(unsigned i=ixlo; i<=ixhi; i++) {
                if (!mask->rowcol(j,i)) continue;
                double yp = inputd.binx(i) - double(*off);
                if (yp >= d.xlow() && yp <= d.xup()) {
		  double z = (_input->content(i,j)-ped)/q;
		  if (z>dl) o->addcontent(z,yp);
		}
              }
            }
          }
          else {
            unsigned ixlo = inputd.xbin(_xlo);
            unsigned ixhi = inputd.xbin(_xhi);
            unsigned iylo = inputd.ybin(_ylo);
            unsigned iyhi = inputd.ybin(_yhi);

            const int16_t* off = _offset;
            for(unsigned j=iylo; j<=iyhi; j++,off++) {
              for(unsigned i=ixlo; i<=ixhi; i++) {
                double yp = inputd.binx(i) - double(*off);
                if (yp >= d.xlow() && yp <= d.xup()) {
		  double z = (_input->content(i,j)-ped)/q;
		  if (z>dl) o->addcontent(z,yp);
		}
              }
            }
          }
	}
	else { // (_axis == Y)
	  const double q  = double(inputd.ppybin());
	  const double dl = _contour.discrimLevel()*double(inputd.ppxbin());
          if (mask) {
            unsigned ixlo = inputd.xbin(_xlo);
            unsigned ixhi = inputd.xbin(_xhi);
            unsigned iylo = inputd.ybin(_ylo);
            unsigned iyhi = inputd.ybin(_yhi);
            for(unsigned j=iylo; j<=iyhi; j++) {
              if (!mask->row(j)) continue;
              const int16_t* off = _offset;
              double y = inputd.biny(j);
              for(unsigned i=ixlo; i<=ixhi; i++) {
                if (!mask->rowcol(j,i)) continue;
                double yp = y - double(*off++);
                if (yp >= d.xlow() && yp <= d.xup()) {
		  double z = (_input->content(i,j)-ped)/q;
		  if (z>dl) o->addcontent(z,yp);
		}
              }
            }
          }
          else {
            unsigned ixlo = inputd.xbin(_xlo);
            unsigned ixhi = inputd.xbin(_xhi);
            unsigned iylo = inputd.ybin(_ylo);
            unsigned iyhi = inputd.ybin(_yhi);
            for(unsigned j=iylo; j<=iyhi; j++) {
              const int16_t* off = _offset;
              double y = inputd.biny(j);
              for(unsigned i=ixlo; i<=ixhi; i++) {
                double yp = y - double(*off++);
                if (yp >= d.xlow() && yp <= d.xup()) {
		  double z = (_input->content(i,j)-ped)/q;
		  if (z>dl) o->addcontent(z,yp);
		}
              }
            }
          }
	}
	o->info(_input->info(EntryImage::Normalization),EntryTH1F::Normalization);
	break; }
    case DescEntry::Prof:  // normalized
      { const DescProf& d = static_cast<const DescProf&>(_routput());
	EntryProf*      o = static_cast<EntryProf*>(_output);
	const double    q = double(inputd.ppxbin()*inputd.ppybin());
	o->reset();
	if (_axis == X) {
          if (mask) {
	    unsigned ixlo = inputd.xbin(_xlo);
	    unsigned ixhi = inputd.xbin(_xhi);
	    unsigned iylo = inputd.ybin(_ylo);
	    unsigned iyhi = inputd.ybin(_yhi);
	    const int16_t* off = _offset;
	    for(unsigned j=iylo; j<=iyhi; j++,off++) {
              if (!mask->row(j)) continue;
	      for(unsigned i=ixlo; i<=ixhi; i++) {
                if (!mask->rowcol(j,i)) continue;
		double yp = inputd.binx(i) - double(*off);
		if (yp >= d.xlow() && yp <= d.xup()) {
		  double z = (_input->content(i,j)-ped)/q;
 		  if (z>_contour.discrimLevel()) o->addy(z,yp);
		}
	      }
	    }
          }
	  else if (inputd.nframes()) {
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
	      int ixlo = inputd.xbin(_xlo);
	      int ixhi = inputd.xbin(_xhi);
	      int iylo = inputd.ybin(_ylo);
	      int iyhi = inputd.ybin(_yhi);
	      const int16_t* off = _offset;
	      if (inputd.xy_bounds(ixlo,ixhi,iylo,iyhi,fn))
		for(int j=iylo; j<iyhi; j++,off++) {
		  for(int i=ixlo; i<ixhi; i++) {
		    double yp = inputd.binx(i) - double(*off);
		    if (yp >= d.xlow() && yp <= d.xup()) {
		      double z = (_input->content(i,j)-ped)/q;
		      if (z>_contour.discrimLevel()) o->addy(z,yp);
		    }
		  }
		}
	    }
	  }
	  else {
	    unsigned ixlo = inputd.xbin(_xlo);
	    unsigned ixhi = inputd.xbin(_xhi);
	    unsigned iylo = inputd.ybin(_ylo);
	    unsigned iyhi = inputd.ybin(_yhi);
	    const int16_t* off = _offset;
	    for(unsigned j=iylo; j<=iyhi; j++,off++) {
	      for(unsigned i=ixlo; i<=ixhi; i++) {
		double yp = inputd.binx(i) - double(*off);
		if (yp >= d.xlow() && yp <= d.xup()) {
		  double z = (_input->content(i,j)-ped)/q;
		  if (z>_contour.discrimLevel()) o->addy(z,yp);
		}
	      }
	    }
	  }
	}
	else { // (_axis == Y)
          if (mask) {
	    unsigned ixlo = inputd.xbin(_xlo);
	    unsigned ixhi = inputd.xbin(_xhi);
	    unsigned iylo = inputd.ybin(_ylo);
	    unsigned iyhi = inputd.ybin(_yhi);
	    for(unsigned j=iylo; j<=iyhi; j++) {
              if (!mask->row(j)) continue;
	      const int16_t* off = _offset;
	      double y = inputd.biny(j);
	      for(unsigned i=ixlo; i<=ixhi; i++,off++) {
                if (!mask->rowcol(j,i)) continue;
		double yp = y - double(*off);
		if (yp >= d.xlow() && yp <= d.xup()) {
		  double z = (_input->content(i,j)-ped)/q;
		  if (z>_contour.discrimLevel()) o->addy(z,yp);
		}
	      }
	    }
          }
	  else if (inputd.nframes()) {
	    for(unsigned fn=0; fn<inputd.nframes(); fn++) {
	      int ixlo = inputd.xbin(_xlo);
	      int ixhi = inputd.xbin(_xhi);
	      int iylo = inputd.ybin(_ylo);
	      int iyhi = inputd.ybin(_yhi);
	      if (inputd.xy_bounds(ixlo,ixhi,iylo,iyhi,fn))
		for(int j=iylo; j<iyhi; j++) {
		  const int16_t* off = _offset;
		  double y = inputd.biny(j);
		  for(int i=ixlo; i<ixhi; i++) {
		    double yp = y - double(*off++);
		    if (yp >= d.xlow() && yp <= d.xup()) {
		      double z = (_input->content(i,j)-ped)/q;
		      if (z>_contour.discrimLevel()) o->addy(z,yp);
		    }
		  }
		}
	    }
	  }
	  else {
	    unsigned ixlo = inputd.xbin(_xlo);
	    unsigned ixhi = inputd.xbin(_xhi);
	    unsigned iylo = inputd.ybin(_ylo);
	    unsigned iyhi = inputd.ybin(_yhi);
	    for(unsigned j=iylo; j<=iyhi; j++) {
	      const int16_t* off = _offset;
	      double y = inputd.biny(j);
	      for(unsigned i=ixlo; i<=ixhi; i++) {
		double yp = y - double(*off++);
		if (yp >= d.xlow() && yp <= d.xup()) {
		  double z = (_input->content(i,j)-ped)/q;
		  if (z>_contour.discrimLevel()) o->addy(z,yp);
		}
	      }
	    }
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

void ContourProjection::_invalid() { _output->invalid(); }
