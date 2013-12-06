#include "RectROI.hh"

#include "ami/data/DescImage.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ImageMask.hh"

#include "ami/data/Cds.hh"

#include <stdio.h>
#include <string.h>

//#define DBUG

using namespace Ami;

RectROI::RectROI(const DescImage& output) :
  AbsOperator(AbsOperator::RectROI),
  _output    (0)
{
  memcpy(_desc_buffer, &output, DESC_LEN);
}

RectROI::RectROI(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::RectROI)
{
  _extract(p, _desc_buffer, DESC_LEN);

  const DescImage& o = *reinterpret_cast<const DescImage*>(_desc_buffer);
  const DescImage& i =  reinterpret_cast<const DescImage&>(input);

  int col0 = int(o.xlow()/i.ppybin());
  int col1 = int(o.xup ()/i.ppybin());
  int row0 = int(o.ylow()/i.ppybin());
  int row1 = int(o.yup ()/i.ppybin());
  int nrows = row1-row0;
  int ncols = col1-col0;
  DescImage desc(o.name(), ncols, nrows,
                 i.ppxbin(), i.ppybin(), 
                 unsigned(o.xlow()), unsigned(o.ylow()));

  desc.aggregate(i.aggregate());
  desc.normalize(i.isnormalized());

  if (i.mask()) {
    desc.set_mask(i.mask()->roi(row0,col0,nrows,ncols));
  }
  else {
    for(unsigned j=0; j<i.nframes(); j++) {
      int x0 = int(o.xlow())/i.ppxbin(), 
        y0 = int(o.ylow())/i.ppybin(), 
        x1 = int(o.xup())/i.ppxbin(),
        y1 = int(o.yup())/i.ppybin();
      if (i.xy_bounds(x0,x1,y0,y1,j)) {
        int nx = x1-x0;
        int ny = y1-y0;
        x0 -= int(o.xlow())/i.ppxbin();
        y0 -= int(o.ylow())/i.ppybin();
        desc.add_frame(x0,y0,nx,ny);
      }
    }
  }

  _output = new EntryImage(desc);
}

RectROI::RectROI(const char*& p) :
  AbsOperator(AbsOperator::RectROI),
  _output    (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
}

RectROI::~RectROI()
{
  if (_output) delete _output;
}

DescEntry& RectROI::_routput   () const 
{ 
  return _output ? _output->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

void*      RectROI::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  return p;
}

Entry&     RectROI::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output;
  
#define SET_BOUNDS(T)                                   \
  T ilo = T((d.xlow()-inputd.xlow())/inputd.ppxbin());  \
  T ihi = T((d.xup ()-inputd.xlow())/inputd.ppxbin());  \
  T jlo = T((d.ylow()-inputd.ylow())/inputd.ppybin());  \
  T jhi = T((d.yup ()-inputd.ylow())/inputd.ppybin());
  
  const EntryImage* _input = static_cast<const EntryImage*>(&e);
  if (_input) {
    const DescImage& inputd = _input->desc();
    const DescImage& d = static_cast<const DescImage&>(_routput());
    EntryImage*      o = static_cast<EntryImage*>(_output);

    const ImageMask*   mask = inputd.mask();
    if (mask) {
      SET_BOUNDS(unsigned) ;
      o->reset();
      for(unsigned i=ilo,ii=0; i<ihi; i++,ii++) {
        if (!mask->col(i)) continue;
        for(unsigned j=jlo,jj=0; j<jhi; j++,jj++) {
          if (mask->rowcol(j,i)) 
            o->content(_input->content(i,j),ii,jj);
        }
      }
    }
    else if (inputd.nframes()) {
      o->reset();
      for(unsigned fn=0; fn<inputd.nframes(); fn++) {
        SET_BOUNDS(int) ;
        ihi--;
        jhi--;
        int iilo = int(d.xlow())/inputd.ppxbin();
        int jjlo = int(d.ylow())/inputd.ppybin();
        if (inputd.xy_bounds(ilo,ihi,jlo,jhi,fn)) { // inclusive
#ifdef DBUG
          printf("RectROI: fn %d: o_nx %d: o_ny %d: jlo %d: jhi %d: jjlo %d\n",
                 fn, o->desc().nbinsx(), o->desc().nbinsy(),
                 jlo, jhi, jjlo);
#endif      
          for(int i=ilo,ii=ilo-iilo; i<=ihi; i++,ii++) {
            for(int j=jlo,jj=jlo-jjlo; j<=jhi; j++,jj++)
              o->content(_input->content(i,j),ii,jj);
          }
        }
      }
    }
    else {
      SET_BOUNDS(unsigned) ;
      for(unsigned i=ilo,ii=0; i<ihi; i++,ii++) {
        for(unsigned j=jlo,jj=0; j<jhi; j++,jj++)
          o->content(_input->content(i,j),ii,jj);
      }
    }

    for(unsigned j=0; j<EntryImage::InfoSize; j++) {
      EntryImage::Info i = (EntryImage::Info)j;
      o->info(_input->info(i),i);
    }
  }

#undef SET_BOUNDS

  _output->valid(e.time());
  return *_output;
}

void RectROI::_invalid() { _output->invalid(); }
