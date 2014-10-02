#ifndef Ami_FIR_hh
#define Ami_FIR_hh

#include "ami/data/AbsOperator.hh"

#include "ndarray/ndarray.h"

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;
  class EntryWaveform;

  class FIR : public AbsOperator {
  public:
    FIR(const char*);
    FIR(const char*&, const DescEntry&);
    ~FIR();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _output!=0; }
    void       _invalid  ();
  private:
    enum { PATH_LEN=256 };
    char           _path[PATH_LEN];
    Entry*            _output;
    ndarray<double,1> _response;
    ndarray<double,1> _outputa;
  };

};

#endif
