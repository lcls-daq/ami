#ifndef Ami_AmiApp_hh
#define Ami_AmiApp_hh

#include <string>
#include <vector>
#include <list>

namespace Ami {

  class AmiApp {
  public:
    static int run(char *partitionTag, unsigned serverGroup, std::vector<char *> module_names, unsigned interface = 0x7f000001, int partitionIndex = 0, bool offline = false);
    static unsigned parse_interface(const char* interface);
    static unsigned parse_ip(const char* ip);
  private:
    template <class U, class C>
    static void load_syms(std::list<U*>& user, char* cnames);
  };

};

#endif
