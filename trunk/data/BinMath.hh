#ifndef Ami_BinMath_hh
#define Ami_BinMath_hh

#include "ami/data/AbsOperator.hh"
#include "ami/data/ScalarPlot.hh"
#include "ami/data/DescEntry.hh"

#include <QtCore/QString>

class QChar;

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;
  class Term;
  class FeatureCache;

  /**
   *  class BinMath : an operator that performs algebra upon the bin values of an entry
   *    and generates a mean value (Scalar), a distribution (TH1F), or a profile (Prof)
   *    versus a BLD or PV quantity.
   */
  class BinMath : public AbsOperator, public ScalarPlot {
  public:
    //  Defined by the input entry's signature, the output entry's description,
    //    the algebraic expression, and any BLD/PV dependence for profiles.
    BinMath(const DescEntry& output, const char* expr);
    //  Reconstituted from the input serial stream, the BldState and PvState
    //    accessors, and the FeatureCaches for input access and output storage.
    BinMath(const char*&, const DescEntry& input, FeatureCache& icache, FeatureCache& ocache);
    BinMath(const char*&);
    ~BinMath();
  public:
    void use();
  public:
    const char*        expression() const;
  public:
    static const QChar& integrate();
    static const QChar& mean     ();
    static const QChar& variance ();
    static const QChar& moment1  ();
    static const QChar& moment2  ();
    static const QChar& contrast ();
    static const QChar& range    ();
    static const QChar& xmoment  ();
    static const QChar& ymoment  ();
    static const double floatPrecision();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
    void       _invalid  ();
    Term*      _process_expr(FeatureCache&,const char*,bool&);
    std::string _text    () const;
  private:
    char*      _cdesc_buffer();
  private:
    enum { EXPRESSION_LEN = 256 };
    char             _expression[EXPRESSION_LEN];
    enum { DESC_LEN = 1024 };
    uint32_t         _desc_buffer[DESC_LEN/sizeof(uint32_t)];

    FeatureCache* _cache;
    Term*         _term;
    mutable const Entry*  _input;
    Entry*        _entry;
    bool          _v;
    mutable unsigned _index;
    bool          _loop;
  };

};

#endif
