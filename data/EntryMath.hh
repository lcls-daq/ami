#ifndef Ami_EntryMath_hh
#define Ami_EntryMath_hh

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;
  class Term;
  class FeatureCache;

  /**
   *   An operator to apply an algebraic expression between objects in the same
   *   analysis thread.  The objects are identified by their index and are assumed
   *   to be of the same shape/size.
   */
  class EntryMath : public AbsOperator {
  public:
    EntryMath(const char* expr);
    EntryMath(const char*&, const DescEntry&, const Cds&, FeatureCache&);
    ~EntryMath();
  public:
    void use();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
    void       _invalid  ();
  private:
    enum { EXPRESSION_LEN = 256 };
    char       _expression[EXPRESSION_LEN];
    Term*      _term;
    Entry*     _entry;
    mutable unsigned   _index;
    bool       _v;
  };

};

#endif
