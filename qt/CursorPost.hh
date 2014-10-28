#ifndef AmiQt_CursorPost_hh
#define AmiQt_CursorPost_hh

#include "ami/qt/SharedData.hh"
#include "ami/qt/CursorOp.hh"

namespace Ami {
  namespace Qt {
    class CPostParent;
    class CursorPost : public SharedData,
		       public CursorOp {
    public:
      CursorPost(unsigned       channel,
		 BinMath*       desc,
                 CPostParent*   parent =0);
      CursorPost(const char*&   p);
      ~CursorPost();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    private:
      CPostParent*       _parent;
    };
  };
};

#endif
		 
