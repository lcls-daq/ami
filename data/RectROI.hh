#ifndef Ami_RectROI_hh
#define Ami_RectROI_hh

//
//  class RectROI
//

#include "ami/data/AbsOperator.hh"
#include "ami/data/DescImage.hh"

namespace Ami {

  class DescEntry;
  class Entry;

  class RectROI : public AbsOperator {
  public:
    RectROI(const DescImage& output);
    RectROI(const char*&, const DescEntry&);
    RectROI(const char*&);
    ~RectROI();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid() const { return true; }
    void       _invalid();
  private:
    enum { DESC_LEN = sizeof(DescImage) };
    uint32_t         _desc_buffer[DESC_LEN/sizeof(uint32_t)];
    Entry*           _output;
  };

};

#endif
