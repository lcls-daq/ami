#ifndef AmiQt_EnvOp_hh
#define AmiQt_EnvOp_hh

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"
#include "ami/data/XML.hh"

#include <map>
#include <string>

namespace Ami {
  class AbsFilter;
  class AbsOperator;
  class DescEntry;
  namespace Qt {
    class EnvOp {
    public:
      EnvOp(const Ami::AbsFilter& filter,
	    DescEntry*            desc,
	    Ami::ScalarSet        set);
      EnvOp();
      ~EnvOp();
    public:
      void save(char*&) const;
      bool load(const Ami::XML::StartTag&, const char*&);
    public:
      const DescEntry& desc() const { return *_desc; }
      void configure(char*& p, unsigned input, unsigned& output, 
		     const AbsOperator& op, bool forceRequest=false);
    protected:
      Ami::AbsFilter*    _filter;
      DescEntry*         _desc;
      Ami::ScalarSet     _set;
      unsigned           _output_signature;
      ConfigureRequestor _req;
    };
  };
};

#endif
