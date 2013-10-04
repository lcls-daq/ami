#include <string.h>

#include "ami/data/Desc.hh"
#include "ami/data/valgnd.hh"

using namespace Ami;

Desc::Desc(const char* name) :
  _signature(-1),
  _nentries(0)
{
  strncpy_val(_name, name, NameSize);
  _name[NameSize-1] = 0;
}

Desc::Desc(const Desc& desc) :
  _signature(desc._signature),
  _nentries (0)
{
  strncpy_val(_name, desc._name, NameSize);
}

Desc::~Desc() {}

const char* Desc::name() const {return _name;}
int Desc::signature() const {return _signature;}

unsigned Desc::nentries() const {return _nentries;}
void Desc::nentries(unsigned n) {_nentries = n;}
void Desc::added() {_nentries++;}
void Desc::reset() {_nentries=0;}
void Desc::signature(int i) {_signature=i;}
void Desc::name(const char* s) { strncpy(_name,s,NameSize); }
