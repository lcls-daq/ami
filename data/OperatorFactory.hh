#ifndef Ami_OperatorFactory_hh
#define Ami_OperatorFactory_hh

namespace Ami {
  class AbsOperator;
  class Cds;
  class DescEntry;
  class Entry;
  class FeatureCache;
  class OperatorFactory {
  public:
    OperatorFactory(FeatureCache& input, FeatureCache& output);
    ~OperatorFactory();
  public:
    AbsOperator* deserialize(const char*&, 
			     const Entry& input,
			     Cds&         output_cds, 
			     unsigned     output_signature) const;
    static AbsOperator* deserialize(const char*&);
  private:
    AbsOperator* _extract(const char*&     p, 
			  const DescEntry& input,
			  Cds&             output_cds) const;
    
    FeatureCache& _input;
    FeatureCache& _output;
  };
};

#endif
