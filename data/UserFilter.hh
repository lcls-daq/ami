#ifndef UserFilter_hh
#define UserFilter_hh

namespace Pds {
  class Src;
  class TypeId;
};

namespace Ami {
  class UserFilter {
  public:
    virtual ~UserFilter() {}
  public:  // Handler functions
    virtual void configure(const Pds::Src&       src,
			   const Pds::TypeId&    type,
			   void*                 payload) = 0;
    virtual void event    (const Pds::Src&       src,
			   const Pds::TypeId&    type,
			   void*                 payload) = 0;
  public:
    virtual bool accept () = 0;
  };
};

typedef Ami::UserFilter* create_f();

#endif
