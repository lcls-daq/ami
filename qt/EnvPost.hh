#ifndef AmiQt_EnvPost_hh
#define AmiQt_EnvPost_hh

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/ConfigureRequestor.hh"

namespace Ami {
  class Cds;
  class DescCache;
  class AbsFilter;
  namespace Qt {
    class EnvPost {
    public:
      EnvPost(const AbsFilter& filter,
              DescCache*       desc,
              Ami::ScalarSet   set);
      EnvPost(const char*&    p);
      ~EnvPost();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void configure(char*& p, unsigned input, unsigned& output);
    private:
      AbsFilter*     _filter;
      DescCache*     _desc;
      Ami::ScalarSet _set;
      unsigned       _output_signature;
      ConfigureRequestor _req;
    };
  };
};

#endif
		 
