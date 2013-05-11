#ifndef Ami_FIR_hh
#define Ami_FIR_hh

#include "ami/data/AbsOperator.hh"

#include <vector>
using std::vector;

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;

  class FIR : public AbsOperator {
  public:
    FIR(const char*);
    FIR(const char*&, const DescEntry&);
    ~FIR();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _output!=0; }
  private:
    enum { PATH_LEN=256 };
    char           _path[PATH_LEN];
    Entry*         _output;
    vector<float>* _response;
  };

};

#endif
