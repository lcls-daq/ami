#ifndef Ami_LeastSquaresFit_hh
#define Ami_LeastSquaresFit_hh

#include "ami/data/LineFitEntry.hh"

namespace Ami {
  class LeastSquaresFit : public LineFitEntry {
  public:
    LeastSquaresFit();
    ~LeastSquaresFit();
  public:
    void fit(double& slope, double& intercept, 
	     double& x0   , double& x1, 
	     const EntryProf&);
    void fit(double& slope, double& intercept,
	     double& x0   , double& x1, 
	     const EntryScan&);
  };
};

#endif
