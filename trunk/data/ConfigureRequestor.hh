#ifndef Ami_ConfigureRequestor_hh
#define Ami_ConfigureRequestor_hh

namespace Ami {

  class ConfigureRequest;

  class ConfigureRequestor {
  public:
    ConfigureRequestor();
    ~ConfigureRequestor();
  public:
    void request(ConfigureRequest& request,
                 unsigned&         output,
                 bool              force=false);
    bool changed() const { return _changed; }
  private:
    char* _prev_request;
    bool  _changed;
  };

};

#endif
