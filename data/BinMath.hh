#ifndef Ami_BinMath_hh
#define Ami_BinMath_hh

//
//  class BinMath : an operator that performs algebra upon the bin values of an entry
//    and generates a mean value (Scalar), a distribution (TH1F), or a profile (Prof)
//    versus a BLD or PV quantity.
//

#include "ami/data/AbsOperator.hh"

class QChar;

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;
  class Term;
  class FeatureCache;

  class BinMath : public AbsOperator {
  public:
    //  Defined by the input entry's signature, the output entry's description,
    //    the algebraic expression, and any BLD/PV dependence for profiles.
    BinMath(unsigned input, const DescEntry& output, const char* expr,
	    unsigned feature_index);
    //  Reconstituted from the input serial stream, the BldState and PvState
    //    accessors, and the Cds input entry accessor.
    BinMath(const char*&, FeatureCache&, const Cds&);
    ~BinMath();
  public:
    unsigned   input    () const;
    DescEntry& output   () const;
    unsigned   feature_index() const;
    const char*        expression() const;
  public:
    static const QChar& integrate();
  private:
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
  private:
    enum { EXPRESSION_LEN = 256 };
    char             _expression[EXPRESSION_LEN];
    unsigned         _signature;
    enum { DESC_LEN = 1024 };
    char             _desc_buffer[DESC_LEN];
    unsigned         _feature_index;

    FeatureCache* _cache;
    Term*         _term;
    Entry*        _entry;
  };

};

#endif
