#ifndef Ami_Analysis_hh
#define Ami_Analysis_hh

#include <list>

namespace Ami {

  class AbsFilter;
  class AbsOperator;
  class Cds;
  class DescEntry;
  class Entry;
  class FeatureCache;

  class Analysis {
  public:
    Analysis(unsigned     id,       // server 
	     const Entry& input,    // input data to analysis
	     unsigned     output,   // output signature
	     Cds&         cds,      // repository
	     FeatureCache& icache,  // scalar features
	     FeatureCache& ocache,  // scalar features
	     const char*& p);       // serial stream
    ~Analysis();
  public:
    unsigned   id     () const;
    void       use    ();
    void       analyze();
    DescEntry& output () const;
    bool       valid  () const;
    const Entry& input() const;
    const AbsOperator& op() const { return *_op; }
  private:
    unsigned     _id;
    unsigned     _output;
    Entry*       _oentry;
    AbsFilter*   _filter;
    AbsOperator* _op;
    const Entry& _input;
    Cds& _cds;
  };

  typedef std::list<Analysis*> AnList;
};

#endif
