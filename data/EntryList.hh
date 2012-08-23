#ifndef Ami_EntryList_hh
#define Ami_EntryList_hh

#include <stdint.h>

namespace Ami {
  class EntryList {
  public:
    enum Option { Empty, Full };
    EntryList(Option = Empty);
    EntryList(uint32_t lower, uint32_t upper);
    EntryList(const EntryList& o) : _m(o._m) {}
  public:
    void insert(unsigned i) { _m |= 1<<i; }
    void remove(unsigned i) { _m &= ~(1<<i); }
    void fill  (unsigned n) { _m=(1<<n)-1; }
    void serialize(uint32_t& lower, uint32_t& upper) const { lower=_m&0xffffffff; upper=_m>>32; }
  public:
    bool contains(unsigned i) const { return _m&(1<<i); }
    uint64_t value() const { return _m; }
  private:
    uint64_t _m;
  };
};

#endif
