#ifndef Ami_GaussFit_hh
#define Ami_GaussFit_hh

#include "ami/data/FitEntry.hh"

#include "ndarray/ndarray.h"

namespace Ami {
  class GaussFit : public FitEntry {
  public:
    GaussFit();
    ~GaussFit();
  public:
    virtual void fit(const EntryTH1F&,double,double);
    virtual void fit(const EntryProf&,double,double);
  public:
    std::vector<double>  params() const;  // Display result
    std::vector<QString> names () const;  // Display result
    const std::vector<double>&  x() const;  // Plot result
    const std::vector<double>&  y() const;
  private:
    std::vector<double> _params;
    std::vector<double> _x;
    std::vector<double> _y;
  };
};

#endif
