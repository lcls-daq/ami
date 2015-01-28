#ifndef Ami_FitEntry_hh
#define Ami_FitEntry_hh

#include "ami/data/Fit.hh"

#include <QtCore/QString>

#include <vector>

namespace Ami {
  class EntryTH1F;
  class EntryProf;

  class FitEntry {
  public:
    virtual ~FitEntry() {}
  public:
    virtual void fit(const EntryTH1F&,double,double)=0;
    virtual void fit(const EntryProf&,double,double)=0;
  public:
    virtual std::vector<double>  params() const=0;  // Display result
    virtual std::vector<QString> names () const=0;  // Display result
    virtual const std::vector<double>&  x() const=0;  // Plot result
    virtual const std::vector<double>&  y() const=0;  // Plot result
  public:
    static FitEntry* instance(Fit::Function, void* args=0);
  };
};

#endif
