#ifndef Ami_Single_hh
#define Ami_Single_hh

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class DescEntry;
  class Entry;
  class FeatureCache;
  class Term;

  class Single : public AbsOperator {
  public:
    Single(const char* =0);
    Single(const char*&, const DescEntry&, FeatureCache&);
    ~Single();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
  private:
    enum { SCALE_LEN=256 };
    char           _scale_buffer[SCALE_LEN];
    Entry*         _entry;
    mutable const Entry* _input;
    Term*          _term ;
  };

};

#endif
