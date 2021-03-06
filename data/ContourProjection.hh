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
		      double xlo, double xhi,
		      double ylo, double yhi);
    ContourProjection(const char*&, const DescEntry&);
    ContourProjection(const char*&);
    ~ContourProjection();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
    void       _invalid  ();
  private:
    enum { DESC_LEN = sizeof(DescProf) };
    uint32_t         _desc_buffer[DESC_LEN/sizeof(uint32_t)];
    Contour          _contour;
    Axis             _axis;
    double           _xlo, _xhi;
    double           _ylo, _yhi;
    Entry*           _output;
    unsigned         _offset_len;
    int16_t*         _offset;
  };

};

#endif
