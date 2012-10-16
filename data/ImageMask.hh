#ifndef Ami_ImageMask_hh
#define Ami_ImageMask_hh

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
  public:
    void clear ();
    void fill  ();
    void invert();
    void clear(unsigned,unsigned);
    void fill (unsigned,unsigned);
    void update();
  private:
    unsigned _nrows;
    unsigned _ncols;
    uint32_t* _rows;
    uint32_t* _cols;
    uint32_t* _rowcol;
  };

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
    unsigned k = i*_ncols+j;
    return _rowcol[k>>5] & (1<<(k&0x1f));
  }
};

#endif
