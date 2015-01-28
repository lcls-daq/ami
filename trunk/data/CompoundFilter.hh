#ifndef Ami_CompoundFilter_hh
#define Ami_CompoundFilter_hh

#include "ami/data/AbsFilter.hh"

namespace Ami {
  class CompoundFilter : public AbsFilter {
  public:
    CompoundFilter(AbsFilter::Type t,AbsFilter& a, AbsFilter& b) : AbsFilter(t), _a(a), _b(b) {}
    ~CompoundFilter() 
    {
      delete &_a;
      delete &_b;
    }
  public:
    const AbsFilter& a() const { return _a; }    
    const AbsFilter& b() const { return _b; }
    void  use   () const { _a.use(); _b.use(); }
  private:
    void* _serialize(void* p) const { return _b.serialize(_a.serialize(p)); }
  protected:
    AbsFilter& _a;
    AbsFilter& _b;
  };
};

#endif
