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
      EnvOp(const Ami::AbsFilter& filter,
            DescEntry*            desc,
            AbsOperator*          op,
	    unsigned              channel);
      EnvOp();
      virtual ~EnvOp();
    public:
      void save(char*&) const;
      bool load(const Ami::XML::StartTag&, const char*&);
    public:
      Ami::ScalarSet   set () const { return Ami::ScalarSet(_set); }
      unsigned      channel() const { return unsigned(_set); }
      const DescEntry& desc() const { return *_desc; }
      const AbsOperator& op() const { return *_op; }
      void configure(char*& p, unsigned input, unsigned& output, 
                     ConfigureRequest::Source source,
		     const AbsOperator& op);
      void configure(char*& p, unsigned input, unsigned& output, 
                     ConfigureRequest::Source source);
    private:
      virtual bool _forceRequest() const { return false; }
    protected:
      Ami::AbsFilter*    _filter;
      DescEntry*         _desc;
      AbsOperator*       _op;
      Ami::ScalarSet     _set;
      unsigned           _channel;
      unsigned           _output_signature;
      ConfigureRequestor _req;
    };
  };
};

#endif
