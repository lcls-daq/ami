#ifndef AmiQt_EnvPost_hh
#define AmiQt_EnvPost_hh

#include "ami/qt/EnvOp.hh"
#include "ami/qt/SharedData.hh"

namespace Ami {
  namespace Qt {
    class EnvPost : public EnvOp,
                    public SharedData {
    public:
      EnvPost(const AbsFilter& filter,
              DescEntry*       desc,
              Ami::ScalarSet   set);
      EnvPost(const AbsFilter& filter,
              DescEntry*       desc,
              AbsOperator*     op,
              unsigned         channel);
      EnvPost(const char*&    p);
      ~EnvPost();
    public:
      void load(const char*& p);
    };
  };
};

#endif
		 
