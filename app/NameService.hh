#ifndef Ami_NameService_hh
#define Ami_NameService_hh

#include "pdsdata/xtc/Xtc.hh"
#include <map>
#include <string>

namespace Ami {
  class NameService {
  public:
    NameService(const Pds::Xtc&);
  public:
    const char* name(const Pds::Src&);
  private:
    std::map<Pds::Src,std::string> _alias;
  };
};

#endif
