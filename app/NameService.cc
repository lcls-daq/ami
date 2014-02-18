#include "ami/app/NameService.hh"

#include "pdsdata/psddl/alias.ddl.h"

namespace Ami {
  class Src : public Pds::Src {
  public:
    Src(const Pds::Src& s) : Pds::Src(s.level()) { _phy = s.phy(); }
  };
};

using namespace Ami;

NameService::NameService() 
{
}

NameService::~NameService() 
{
}

void NameService::clear()
{
  _alias.clear();
}

void NameService::append(const Pds::Xtc& xtc)
{
  if (xtc.contains.id()==Pds::TypeId::Id_AliasConfig) {
    switch(xtc.contains.version()) {
    case 1:
      { const Pds::Alias::ConfigV1& c = *reinterpret_cast<const Pds::Alias::ConfigV1*>(xtc.payload());
	ndarray<const Pds::Alias::SrcAlias,1> a = c.srcAlias();
	for(const Pds::Alias::SrcAlias* p = a.begin(); p!=a.end(); p++) {
	  _alias[Ami::Src(p->src())] = std::string(p->aliasName());
	}
      } break;
    default:
      break;
    }
  }
}

const char* NameService::name(const Pds::Src& s)
{
  Ami::Src src(s);
  std::map<Pds::Src,std::string>::iterator it=_alias.find(src);
  return (it == _alias.end()) ? 0 : it->second.c_str();
}
