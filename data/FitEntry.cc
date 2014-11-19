#include "ami/data/FitEntry.hh"
#include "ami/data/GaussFit.hh"
#include "ami/data/LorentzFit.hh"

using namespace Ami;

FitEntry* FitEntry::instance(Fit::Function m, void* args)
{
  switch(m) {
  case Fit::Gauss  : return new GaussFit;
  case Fit::Lorentz: return new LorentzFit;
  default: abort(); 
  }
}
