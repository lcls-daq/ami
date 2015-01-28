#ifndef Ami_MedianSlopeFit_hh
#define Ami_MedianSlopeFit_hh

#include "ami/data/LineFitEntry.hh"

#include "ndarray/ndarray.h"

namespace Ami {
  class MedianSlopeFit : public LineFitEntry {
  public:
    MedianSlopeFit();
    ~MedianSlopeFit();
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
