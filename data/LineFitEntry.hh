#ifndef Ami_LineFitEntry_hh
#define Ami_LineFitEntry_hh

#include "ami/data/LineFit.hh"

namespace Ami {
  class EntryScan;
  class EntryProf;

  class LineFitEntry {
  public:
    virtual ~LineFitEntry() {}
  public:
    virtual void fit(double& slope, double& intercept, 
		     double& x0   , double& x1, 
		     const EntryProf&)=0;
    virtual void fit(double& slope, double& intercept, 
		     double& x0   , double& x1, 
		     const EntryScan&)=0;
  public:
    static LineFitEntry* instance(LineFit::Method);
  };
};

#endif
