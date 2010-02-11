#ifndef Ami_ContourProjection_hh
#define Ami_ContourProjection_hh

//
//  class ContourProjection : an operator that projects an EntryImage onto the
//    y-axis after subtracting an x-dependent offset
//

#include "ami/data/AbsOperator.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/Contour.hh"

namespace Ami {

  class DescEntry;
  class Entry;

  class ContourProjection : public AbsOperator {
  public:
    enum Axis { X, Y };
    ContourProjection(const DescEntry& output,
		      const Contour&,Axis, 
		      unsigned ilo, unsigned ihi,
		      unsigned jlo, unsigned jhi);
    ContourProjection(const char*&, const DescEntry&);
    ContourProjection(const char*&);
    ~ContourProjection();
  public:
    DescEntry& output   () const;
  private:
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
  private:
    enum { DESC_LEN = sizeof(DescProf) };
    char             _desc_buffer[DESC_LEN];
    Contour          _contour;
    Axis             _axis;
    unsigned         _ilo, _ihi;
    unsigned         _jlo, _jhi;
    Entry*           _output;
    unsigned         _offset_len;
    int16_t*         _offset;
  };

};

#endif
