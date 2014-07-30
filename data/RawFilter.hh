#ifndef Ami_RawFilter_hh
#define Ami_RawFilter_hh

#include "AbsFilter.hh"

namespace Ami {
  class RawFilter : public AbsFilter {
  public:
    RawFilter();
    ~RawFilter();
  public:
    bool valid () const;
    bool accept() const;
    AbsFilter* clone() const;
    std::string text() const;
  private:
    void* _serialize(void*) const;
  };
};

#endif
