#ifndef Ami_LSNPolyFit_hh
#define Ami_LSNPolyFit_hh

#include "ami/data/FitEntry.hh"

namespace Ami {
  class LSNPolyFit : public FitEntry {
  public:
    LSNPolyFit(unsigned order);
    ~LSNPolyFit();
  public:
    void fit(const EntryProf&);
    void fit(const EntryScan&);
  public:
    std::vector<double> params() const;
    std::vector<QString> names() const;
    double value(double x) const;
  };
};

#endif
