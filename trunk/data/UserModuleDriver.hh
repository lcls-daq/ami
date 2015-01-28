#ifndef UserModuleDriver_hh
#define UserModuleDriver_hh

namespace Ami {
  class UserModule;
  class UserModuleDriver {
  public:
    virtual ~UserModuleDriver() {}
  public:
    virtual void recreate(UserModule*) = 0;
  };
};

#endif
