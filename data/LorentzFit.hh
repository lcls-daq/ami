#ifndef Ami_LorentzFit_hh
#define Ami_LorentzFit_hh

#include "ami/data/FitEntry.hh"

#include "ndarray/ndarray.h"

namespace Ami {
  class LorentzFit : public FitEntry {
  public:
    LorentzFit();
    ~LorentzFit();
  public:
    virtual void fit(const EntryTH1F&);
    virtual void fit(const EntryProf&);
  public:
    std::vector<double>  params() const;  // Display result
    std::vector<QString> names () const;  // Display result
    const std::vector<double>&  x() const;  // Plot result
    const std::vector<double>&  y() const;
  private:
    void _fit(const std::vector<double>&,
              const std::vector<double>&,
              const std::vector<double>&,
              double);
  private:
    std::vector<double> _params;
    std::vector<double> _x;
    std::vector<double> _y;
    bool _converged;
  };
};

#endif
