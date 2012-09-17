#ifndef Ami_EntryRefOp_hh
#define Ami_EntryRefOp_hh

//
//  This operation must always be serialized in front of 
//  another operator, because it creates no unique Entry
//  of its own (note the implementation of ::output).
//

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class Cds;
  class DescEntry;

  class EntryRefOp : public AbsOperator {
  public:
    EntryRefOp (unsigned index);
    EntryRefOp (const char*&, const DescEntry&);
    ~EntryRefOp();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
  private:
    unsigned       _index;
    DescEntry*     _output;
  };

};

#endif
