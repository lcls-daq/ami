#include "ami/data/Cdu.hh"
#include "ami/data/Cds.hh"

using namespace Ami;

Cdu::Cdu() : _cds(0) {}
    
Cdu::~Cdu() { if (_cds) _cds->unsubscribe(*this); }

void Cdu::subscribe(Cds& cds) 
{ 
  if (_cds && _cds!=&cds) _cds->unsubscribe(*this);
  (_cds=&cds)->subscribe(*this); 
}
