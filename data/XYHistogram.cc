#include "XYHistogram.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/ImageMask.hh"

#include "ami/data/Cds.hh"

#include <stdio.h>

using namespace Ami;

XYHistogram::XYHistogram(const DescEntry& output) :
  AbsOperator(AbsOperator::XYHistogram),
  _output    (0)
{
  memcpy(_desc_buffer, &output, output.size());
  memset(_desc_buffer+output.size(), 0, DESC_LEN-output.size());
}

XYHistogram::XYHistogram(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::XYHistogram)
{
  _extract(p, _desc_buffer, DESC_LEN);

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);
  _output = EntryFactory::entry(o);
}

XYHistogram::XYHistogram(const char*& p) :
  AbsOperator(AbsOperator::XYHistogram),
  _output    (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
}

XYHistogram::~XYHistogram()
{
  if (_output) delete _output;
}

DescEntry& XYHistogram::_routput   () const 
{ 
  return _output ? _output->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

void*      XYHistogram::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  return p;
}

Entry&     XYHistogram::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output;

  const EntryImage* _input = static_cast<const EntryImage*>(&e);
  const DescImage& inputd = _input->desc();
  const ImageMask* mask = inputd.mask();
  if (_input) {

    unsigned ilo(0), ihi(inputd.nbinsx());
    unsigned jlo(0), jhi(inputd.nbinsy());
    
    double   p(_input->info(EntryImage::Pedestal));
    double   n   = 1./double(inputd.ppxbin()*inputd.ppybin());
    
    switch(_routput().type()) {
    case DescEntry::TH1F:  // unnormalized
      { EntryTH1F*      o = static_cast<EntryTH1F*>(_output);
        o->clear();
        if (mask) {
          for(unsigned j=jlo; j<jhi; j++) {
            if (!mask->row(j)) continue;
            for(unsigned i=ilo; i<ihi; i++)
              if (mask->rowcol(j,i))
                o->addcontent(1.,(double(_input->content(i,j))-p)*n);
          }
        }
        else if (inputd.nframes()) {
          for(int fn=0; fn<int(inputd.nframes()); fn++) {
            int i0(ilo),i1(ihi),j0(jlo),j1(jhi);
            if (inputd.xy_bounds(i0,i1,j0,j1,fn)) {
              for(int j=j0; j<=j1; j++) {
                for(int i=i0; i<=i1; i++) {
                  o->addcontent(1.,(double(_input->content(i,j))-p)*n);
                }
              }
            }
          }
        } else {
          for(unsigned j=jlo; j<jhi; j++)
            for(unsigned i=ilo; i<ihi; i++)
              o->addcontent(1.,(double(_input->content(i,j))-p)*n);
        }
        o->info(_input->info(EntryImage::Normalization),EntryTH1F::Normalization);
        break; }
    case DescEntry::ScalarRange:
      { EntryScalarRange* o = static_cast<EntryScalarRange*>(_output);
        if (mask) {
          for(unsigned j=jlo; j<jhi; j++) {
            if (!mask->row(j)) continue;
            for(unsigned i=ilo; i<ihi; i++)
              if (mask->rowcol(j,i))
                o->addcontent((double(_input->content(i,j))-p)*n);
          }
        }
        else if (inputd.nframes()) {
          for(int fn=0; fn<int(inputd.nframes()); fn++) {
            int i0(ilo),i1(ihi),j0(jlo),j1(jhi);
            if (inputd.xy_bounds(i0,i1,j0,j1,fn)) {
              for(int j=j0; j<=j1; j++) {
                for(int i=i0; i<=i1; i++) {
                  o->addcontent((double(_input->content(i,j))-p)*n);
                }
              }
            }
          }
        } else {
          for(unsigned j=jlo; j<jhi; j++)
            for(unsigned i=ilo; i<ihi; i++)
              o->addcontent((double(_input->content(i,j))-p)*n);
        }
        break; }
    default:
      break;
    }
  }
  _output->valid(e.time());
  return *_output;
}
