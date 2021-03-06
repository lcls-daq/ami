#ifndef Ami_XYProjection_hh
#define Ami_XYProjection_hh

//
//  class XYProjection : an operator that projects an EntryImage onto an axis
//    (XYProjection::Axis) over a range of the other axis.
//

#include "ami/data/AbsOperator.hh"
#include "ami/data/DescProf.hh"

namespace Ami {

  class DescEntry;
  class Entry;

  class XYProjection : public AbsOperator {
  public:
    enum Axis { X, Y };
    XYProjection(const DescEntry& output, Axis);
    XYProjection(const char*&, const DescEntry&);
    XYProjection(const char*&);
    ~XYProjection();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid() const { return true; }
    void       _invalid();
  private:
    enum { DESC_LEN = sizeof(DescProf) };
    uint32_t         _desc_buffer[DESC_LEN/sizeof(uint32_t)];
    Axis             _axis;
    Entry*           _output;
  };

};

#endif
