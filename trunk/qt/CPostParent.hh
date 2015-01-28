#ifndef AmiQt_CPostParent_hh
#define AmiQt_CPostParent_hh

namespace Ami {
  namespace Qt {
    class CursorPost;
    class CPostParent {
    public:
      virtual ~CPostParent() {}
    public:
      virtual void remove_cursor_post(CursorPost*) =0;
    };
  };
};

#endif
