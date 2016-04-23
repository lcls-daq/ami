#include "RotateImage.hh"

#include "ami/data/EntryFactory.hh"
#include "ami/data/EntryImage.hh"

#include "ami/data/Cds.hh"
#include "ami/data/valgnd.hh"

#include <stdio.h>

using namespace Ami;

#define SWAP(a,b) { int u=a; a=b; b=u; }

RotateImage::RotateImage(const DescEntry& output, Rotation r) :
  AbsOperator(AbsOperator::RotateImage),
  _rotation  (r),
  _output    (0)
{
  memcpy_val(_desc_buffer, &output, output.size(),DESC_LEN);

  DescImage& o = *reinterpret_cast<DescImage*>(_desc_buffer);
  switch(_rotation) {
  case Ami::D90:
  case Ami::D270:
    SWAP(o._nbinsx,o._nbinsy);
    SWAP(o._ppbx  ,o._ppby);
    SWAP(o._dpbx  ,o._dpby);
    SWAP(o._xp0   ,o._yp0);
    { float u=o._mmppx; o._mmppx=o._mmppy; o._mmppy=u; }
    break;
  default:
    break;
  }
}

RotateImage::RotateImage(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::RotateImage)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_rotation  , sizeof(_rotation));

  const DescImage& o = *reinterpret_cast<const DescImage*>(_desc_buffer);
  const DescImage& d =  reinterpret_cast<const DescImage&>(input);

  DescEntry* e = new DescImage(o);

  if (e) {
    e->aggregate(d.aggregate());
    _output = EntryFactory::entry(o);
    delete e;
  }
  else
    _output = 0;
}

RotateImage::RotateImage(const char*& p) :
  AbsOperator(AbsOperator::RotateImage),
  _output    (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_rotation  , sizeof(_rotation));
}

RotateImage::~RotateImage()
{
  if (_output) delete _output;
}

const DescEntry& RotateImage::_routput   () const 
{ 
  return _output ? _output->desc() : *reinterpret_cast<const DescEntry*>(_desc_buffer); 
}

void*      RotateImage::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_rotation  , sizeof(_rotation));
  return p;
}

Entry&     RotateImage::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output;

  const EntryImage* _input = static_cast<const EntryImage*>(&e);
  const DescImage& inputd = _input->desc();

  if (_input) {
    EntryImage& output = *static_cast<EntryImage*>(_output);
    for(unsigned y=0; y<inputd.nbinsy(); y++) {
      const unsigned* row = &_input->content()[y][0];
      switch(_rotation) {
      case D0: 
        { unsigned iy = y;
          for(unsigned i=0, ix=0; i<inputd.nbinsx(); i++, ix--)
          output.content(row[i],ix,iy);
        } break;
      case D90:
        { unsigned iy = y;
          for(unsigned i=0, ix=inputd.nbinsx()-1; i<inputd.nbinsx(); i++, ix--)
            output.content(row[i],iy,ix);
        } break;
      case D180:
        { unsigned iy = (inputd.nbinsy()-1-y);
          for(unsigned i=0, ix=inputd.nbinsx()-1; i<inputd.nbinsx(); i++, ix--)
            output.content(row[i],ix,iy);
        } break;
      case D270:
        { unsigned iy = (inputd.nbinsy()-1-y);
          for(unsigned i=0, ix=0; i<inputd.nbinsx(); i++, ix++)
            output.content(row[i],iy,ix);
        } break;
      default:
        break;
      }
    }
    for(unsigned i=0; i<EntryImage::InfoSize; i++)
      output.info (_input->info((EntryImage::Info)i),(EntryImage::Info)i);
  }

  _output->valid(e.time());
  return *_output;
}

void RotateImage::_invalid() { _output->invalid(); }
