#ifndef Ami_SingleShot_HH
#define Ami_SingleShot_HH

//
//  SingleShot should be the last filter in a sequence;
//  it will fire only once.
//

#include "ami/data/AbsFilter.hh"

namespace Ami {

  class SingleShot : public AbsFilter {
  public:
    SingleShot();
    ~SingleShot();
  public:
    bool  valid () const;
    bool  accept() const;
    AbsFilter* clone() const;
    std::string text() const;
  private:
    void* _serialize(void*) const;
  public:
    bool _fired;
  };
};

#endif
