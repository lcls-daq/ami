#include "ImageMask.hh"

#include "ami/data/DescImage.hh"

#include <string.h>
#include <stdio.h>

using namespace Ami;

ImageMask::ImageMask(unsigned rows, unsigned cols) :
  _nrows(rows),
  _ncols(cols)
{
  unsigned rsz = (rows+31)>>5;
  unsigned csz = (cols+31)>>5;
  unsigned sz  = (rows*cols+31)>>5;

  _rows    = new uint32_t[rsz];
  _cols    = new uint32_t[csz];
  _rowcol  = new uint32_t[sz];
}

ImageMask::ImageMask(unsigned rows, unsigned cols, 
                     unsigned nframes, const SubFrame* frames,
                     const char* path) :
  _nrows(rows),
  _ncols(cols)
{
  unsigned rsz = (rows+31)>>5;
  unsigned csz = (cols+31)>>5;
  unsigned sz  = (rows*cols+31)>>5;
  uint32_t* p  = new uint32_t[sz];

  //
  //  Fill the mask with the SubFrame areas
  //
  if (nframes) {
    memset(p, 0, sz<<2);
    for(unsigned i=0; i<nframes; i++)
      for(unsigned j=0; j<frames[i].ny; j++) {
        uint32_t m = (frames[i].y+j)*cols + frames[i].x;
        uint32_t q = m + frames[i].nx;
        unsigned im = m>>5;
        unsigned iq = q>>5;
        m &= 0x1f;
        q &= 0x1f;

        p[im] |= ~((1<<m)-1);
        while( ++im < iq )
          p[im] = 0xffffffff;
        
        p[iq] |= (1<<q)-1;
      }
  }
  else {
    memset(p, 0xff, sz<<2);
  }

  _rows    = new uint32_t[rsz];
  _cols    = new uint32_t[csz];
  _rowcol  = new uint32_t[sz];

  memset(_rows, 0, rsz<<2);
  memset(_cols, 0, csz<<2);
  memset(_rowcol, 0, sz<<2);

  FILE* f = fopen(path,"r");
  if (f) {
    //
    //  AND with the mask read from the file
    //
    float v;
    for(unsigned i=0,k=0; i<rows; i++) {
      unsigned rb = 1<<(i&0x1f);
      for(unsigned j=0; j<cols; j++,k++) {
        fscanf(f, "%f", &v);
        unsigned ib = 1<<(k&0x1f);
        if (v && (p[k>>5]&ib)) {
          _rowcol[k>>5] |= ib;
          _rows  [i>>5] |= rb;
          _cols  [j>>5] |= 1<<(j&0x1f);
        }
        else
          _rowcol[k>>5] &= ~ib;
      }
    }

    fclose(f);
  }
  else {
    for(unsigned i=0,k=0; i<rows; i++) {
      unsigned rb = 1<<(i&0x1f);
      for(unsigned j=0; j<cols; j++,k++) {
        unsigned ib = 1<<(k&0x1f);
        if (p[k>>5]&ib) {
          _rowcol[k>>5] |= ib;
          _rows  [i>>5] |= rb;
          _cols  [j>>5] |= 1<<(j&0x1f);
        }
        else
          _rowcol[k>>5] &= ~ib;
      }
    }
  }

  delete[] p;
}

ImageMask::ImageMask(const ImageMask& m) :
  _nrows(m._nrows),
  _ncols(m._ncols)
{
  unsigned rsz = (_nrows+31)>>5;
  unsigned csz = (_ncols+31)>>5;
  unsigned sz  = (_nrows*_ncols+31)>>5;

  _rows    = new uint32_t[rsz];
  _cols    = new uint32_t[csz];
  _rowcol  = new uint32_t[sz];

  memcpy(_rows, m._rows, rsz<<2);
  memcpy(_cols, m._cols, csz<<2);
  memcpy(_rowcol, m._rowcol, sz<<2);
}

ImageMask::~ImageMask()
{
  delete[] _rows;
  delete[] _cols;
  delete[] _rowcol;
}

void ImageMask::clear()
{
  unsigned rsz = (_nrows+31)>>5;
  unsigned csz = (_ncols+31)>>5;
  unsigned sz  = (_nrows*_ncols+31)>>5;

  memset(_rows, 0, rsz<<2);
  memset(_cols, 0, csz<<2);
  memset(_rowcol, 0, sz<<2);
}

void ImageMask::fill()
{
  unsigned rsz = (_nrows+31)>>5;
  unsigned csz = (_ncols+31)>>5;
  unsigned sz  = (_nrows*_ncols+31)>>5;

  memset(_rows, 0xff, rsz<<2);
  memset(_cols, 0xff, csz<<2);
  memset(_rowcol, 0xff, sz<<2);
}

void ImageMask::clear(unsigned row, unsigned col)
{
  unsigned k = row*_ncols+col;
  _rowcol[k>>5] &= ~(1<<(k&0x1f));
}

void ImageMask::fill(unsigned row, unsigned col)
{
  unsigned k = row*_ncols+col;
  _rowcol[k>>5] |= (1<<(k&0x1f));
}

void ImageMask::invert()
{
  unsigned rsz = (_nrows+31)>>5;
  unsigned csz = (_ncols+31)>>5;
  unsigned sz  = (_nrows*_ncols+31)>>5;

  memset(_rows, 0, rsz<<2);
  memset(_cols, 0, csz<<2);

  for(unsigned i=0; i<sz; i++)
    _rowcol[i] = ~_rowcol[i];

  for(unsigned i=0,k=0; i<_nrows; i++) {
    unsigned rb = 1<<(i&0x1f);
    for(unsigned j=0; j<_ncols; j++,k++) {
      unsigned ib = 1<<(k&0x1f);
      if (_rowcol[k>>5]&ib) {
        _rows  [i>>5] |= rb;
        _cols  [j>>5] |= 1<<(j&0x1f);
      }
    }
  }
}

void ImageMask::update()
{
  memset(_rows, 0, ((_nrows+31)>>5)<<2); 
  memset(_cols, 0, ((_ncols+31)>>5)<<2); 

  for(unsigned j=0,k=0; j<_nrows; j++,k++) { 
    unsigned jb = 1<<(j&0x1f); 
    for(unsigned i=0; i<_ncols; i++) { 
      unsigned ib = 1<<(i&0x1f);
      unsigned kb = 1<<(k&0x1f);
      if (_rowcol[k>>5]&kb) {
        _rows[j>>5] |= jb;
        _cols[i>>5] |= ib;
      }
    }
  }
}

ImageMask ImageMask::operator ~ () const {
  ImageMask m(*this);
  m.invert();
  return m;
}

#define MASK_OPERATOR(o)                                        \
  ImageMask& ImageMask::operator o (const ImageMask& m) {       \
    if (m.rows()!=_nrows ||                                     \
        m.cols()!=_ncols)                                       \
      return *this;                                             \
                                                                \
    unsigned sz = (_nrows*_ncols+31)>>5;                        \
    for(unsigned k=0; k<sz; k++)                                \
      _rowcol[k] o m._rowcol[k];                                \
                                                                \
    update();                                                   \
    return *this;                                               \
  }

MASK_OPERATOR(=)
MASK_OPERATOR(&=)
MASK_OPERATOR(|=)
MASK_OPERATOR(^=)

