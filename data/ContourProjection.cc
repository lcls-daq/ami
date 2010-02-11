#include "ContourProjection.hh"

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

ContourProjection::ContourProjection(const DescEntry& output, 
				     const Contour& contour,
				     Axis axis, 
				     unsigned ilo, unsigned ihi,
				     unsigned jlo, unsigned jhi) :
  AbsOperator(AbsOperator::ContourProjection),
  _contour   (contour),
  _axis      (axis),
  _ilo       (ilo),
  _ihi       (ihi),
  _jlo       (jlo),
  _jhi       (jhi),
  _output    (0),
  _offset_len(0),
  _offset    (new int16_t[2])
{
  memcpy(_desc_buffer, &output, output.size());
}

ContourProjection::ContourProjection(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::ContourProjection)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_contour   , sizeof(_contour));
  _extract(p, &_axis      , sizeof(_axis));
  _extract(p, &_ilo       , sizeof(_ilo ));
  _extract(p, &_ihi       , sizeof(_ihi ));
  _extract(p, &_jlo       , sizeof(_jlo ));
  _extract(p, &_jhi       , sizeof(_jhi ));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);
  _output = EntryFactory::entry(o);

  _offset_len = 0;
  _offset     = new int16_t[2];
}

ContourProjection::ContourProjection(const char*& p) :
  AbsOperator(AbsOperator::ContourProjection),
  _output    (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_contour   , sizeof(_contour));
  _extract(p, &_axis      , sizeof(_axis));
  _extract(p, &_ilo       , sizeof(_ilo ));
  _extract(p, &_ihi       , sizeof(_ihi ));
  _extract(p, &_jlo       , sizeof(_jlo ));
  _extract(p, &_jhi       , sizeof(_jhi ));

  _offset_len = 0;
  _offset     = new int16_t[2];
}

ContourProjection::~ContourProjection()
{
  if (_output) delete _output;
  if (_offset) delete[] _offset;
}

DescEntry& ContourProjection::output   () const 
{ 
  return _output ? _output->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

void*      ContourProjection::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_contour   , sizeof(_contour));
  _insert(p, &_axis      , sizeof(_axis));
  _insert(p, &_ilo       , sizeof(_ilo ));
  _insert(p, &_ihi       , sizeof(_ihi ));
  _insert(p, &_jlo       , sizeof(_jlo ));
  _insert(p, &_jhi       , sizeof(_jhi ));
  return p;
}

Entry&     ContourProjection::_operate(const Entry& e) const
{
  const EntryImage* _input = static_cast<const EntryImage*>(&e);
  const DescImage& inputd  = _input->desc();
  ContourProjection* pthis = const_cast<ContourProjection*>(this);
  if (_input) {
    if      (_axis==Y && inputd.nbinsx()==_offset_len) ;
    else if (_axis==X && inputd.nbinsy()==_offset_len) ;
    else if (_axis==Y) {
      pthis->_offset_len = inputd.nbinsx();
      pthis->_offset = new int16_t[_offset_len];
      float x = inputd.xlow()+0.5*float(inputd.ppxbin());
      for(unsigned i=0; i<_offset_len; i++) {
	pthis->_offset[i] = float(_contour.value(x));
	x += float(inputd.ppxbin());
      }
    }
    else if (_axis==X) {
      pthis->_offset_len = inputd.nbinsy();
      pthis->_offset = new int16_t[_offset_len];
      float x = inputd.ylow()+0.5*float(inputd.ppybin());
      for(unsigned i=0; i<_offset_len; i++) {
	pthis->_offset[i] = float(_contour.value(x));
	x += float(inputd.ppybin());
      }
    }
    switch(output().type()) {
    case DescEntry::TH1F:  // unnormalized
      { const DescTH1F& d = static_cast<const DescTH1F&>(output());
	EntryTH1F*      o = static_cast<EntryTH1F*>(_output);
	o->clear();
	for(unsigned j=_jlo; j<_jhi; j++) {
	  if (_axis == X) {
	    int16_t off = _offset[j];
	    for(unsigned i=_ilo; i<_ihi; i++) {
	      double v = i - off;
	      if (v >= d.xlow() && v <= d.xup())
		o->addcontent(_input->content(i,j),v);
	    }
	  }
	  else { // (_axis == Y)
	    for(unsigned i=_ilo; i<_ihi; i++) {
	      double v = j - _offset[i];
	      if (v >= d.xlow() && v <= d.xup())
		o->addcontent(_input->content(i,j),v);
	    }
	  }
	}
	o->info(_input->info(EntryImage::Normalization),EntryTH1F::Normalization);
	break; }
    case DescEntry::Prof:  // normalized
      { const DescProf& d = static_cast<const DescProf&>(output());
	EntryProf*      o = static_cast<EntryProf*>(_output);
	o->reset();
	for(unsigned j=_jlo; j<_jhi; j++) {
	  if (_axis == X) {
	    int16_t off = _offset[j];
	    for(unsigned i=_ilo; i<_ihi; i++) {
	      double v = i - off;
	      if (v >= d.xlow() && v <= d.xup())
		o->addy(_input->content(i,j),v);
	    }
	  }
	  else { // (_axis == Y)
	    for(unsigned i=_ilo; i<_ihi; i++) {
	      double v = j - _offset[i];
	      if (v >= d.xlow() && v <= d.xup())
		o->addy(_input->content(i,j),v);
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
