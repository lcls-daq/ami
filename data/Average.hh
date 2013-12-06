#ifndef Ami_Average_hh
#define Ami_Average_hh

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;
  class FeatureCache;
  class Term;

  /**
   *   An operator to average over events, either a fixed number or 
   *   a running sum of all events.
   */
  class Average : public AbsOperator {
  public:
    Average(unsigned n=0, const char* p=0);
    Average(const char*&, const DescEntry&, FeatureCache&);
    ~Average();
  public:
    void use();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
    void       _invalid  ();
  private:
    enum { SCALE_LEN=256 };
    char           _scale_buffer[SCALE_LEN];
    unsigned       _n;
    mutable unsigned       _current;
    Entry*         _entry;
    Entry*         _cache;
    mutable const Entry* _input;
    Term*          _term ;
    bool           _v;
  };

};

#endif
