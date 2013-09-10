#include "ami/app/NameService.hh"

#include "pdsdata/psddl/alias.ddl.h"

using namespace Ami;

NameService::NameService(const Pds::Xtc& xtc)
{
  if (xtc.contains.id()==Pds::TypeId::Id_AliasConfig) {
    switch(xtc.contains.version()) {
    case 1:
      { const Pds::Alias::ConfigV1& c = *reinterpret_cast<const Pds::Alias::ConfigV1*>(xtc.payload());
	_alias.clear();
	ndarray<const Pds::SrcAlias,1> a = c.srcAlias();
	for(const Pds::SrcAlias* p = a.begin(); p!=a.end(); p++) {
	  _alias[*p] = std::string(p->aliasName());
	}
      } break;
    default:
      break;
    }
  }
}

const char* NameService::name(const Pds::Src& src)
{
  std::map<Pds::Src,std::string>::iterator it=_alias.find(src);
  return (it == _alias.end()) ? 0 : it->second.c_str();
}
