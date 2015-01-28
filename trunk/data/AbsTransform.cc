#include "ami/data/AbsTransform.hh"

namespace Ami {
  class NullTransform : public AbsTransform {
  public:
    ~NullTransform() {}
    double operator()(double x) const { return x; }
  };
};

using namespace Ami;

static NullTransform _nullTransform;

AbsTransform& AbsTransform::null() { return _nullTransform; }
