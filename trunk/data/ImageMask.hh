#ifndef Ami_ImageMask_hh
#define Ami_ImageMask_hh

#include "ndarray/ndarray.h"

#include <stdint.h>

namespace Ami {
  class SubFrame;
  class ImageMask {
  public:
    ImageMask(unsigned rows, unsigned cols);
    ImageMask(unsigned rows, unsigned cols, 
              unsigned nframes, const SubFrame* frames,
              const char*);
    ImageMask(const ImageMask&);
    ~ImageMask();
  public:
    ImageMask& operator  = (const ImageMask&);
    ImageMask& operator &= (const ImageMask&);
    ImageMask& operator |= (const ImageMask&);
    ImageMask& operator ^= (const ImageMask&);
    ImageMask  operator ~  () const;
  public:
    unsigned rows() const { return _nrows; }
    unsigned cols() const { return _ncols; }
    bool row(unsigned i) const;
    bool col(unsigned j) const;
    bool rowcol(unsigned i,unsigned j) const;
    ImageMask roi(unsigned row0, unsigned col0,
                  unsigned rows, unsigned cols) const;
    ndarray<unsigned,1> row_mask() const;
    ndarray<unsigned,2> all_mask() const;
  public:
    void clear ();
    void fill  ();
    void invert();
    void clear(unsigned,unsigned);
    void fill (unsigned,unsigned);
    void update();
    void write(FILE*) const;
    void dump() const;
  private:
    unsigned _rowsz() const;
  private:
    unsigned _nrows;
    unsigned _ncols;
    uint32_t* _rows;
    uint32_t* _cols;
    uint32_t* _rowcol;
  };

  inline unsigned ImageMask::_rowsz() const
  {
    return (_ncols+31)>>5;
  }

  inline bool ImageMask::row(unsigned i) const
  {
    return _rows[i>>5] & (1<<(i&0x1f)); 
  }

  inline bool ImageMask::col(unsigned i) const
  {
    return _cols[i>>5] & (1<<(i&0x1f)); 
  }

  inline bool ImageMask::rowcol(unsigned i, unsigned j) const 
  {
    return _rowcol[i*_rowsz()+(j>>5)] & (1<<(j&0x1f));
  }

  inline ndarray<unsigned,1> ImageMask::row_mask() const 
  {
    unsigned shape[] = {_nrows};
    return ndarray<unsigned,1>(_rows,shape);
  }
  
  inline ndarray<unsigned,2> ImageMask::all_mask() const
  {
    unsigned shape[] = {_nrows,_rowsz()};
    return ndarray<unsigned,2>(_rowcol,shape);
  }

};

#endif
