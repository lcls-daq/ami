#include "ami/data/NLineFitEntry.hh"
#include "ami/data/LSNPolyFit.hh"

using namespace Ami;

NLineFitEntry* NLineFitEntry::instance(NLineFit::Method m, void* args)
{
  switch(m) {
  case NLineFit::LSNPolyFit: return new LSNPolyFit(*(unsigned*)args);
  default: abort(); 
  }
}
