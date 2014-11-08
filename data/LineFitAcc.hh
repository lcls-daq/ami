#ifndef Ami_LineFitAcc_hh
#define Ami_LineFitAcc_hh

namespace Ami {
  class EntryScalar;
  class EntryScan;
  class EntryProf;
  class EntryProf2D;

  class LineFitAcc {
  public:
    virtual ~LineFitAcc() {}
  public:
    virtual void add(EntryScalar&, double x, double y)=0;
    virtual void add(EntryScan&  , double x, double y, double vx, double vt)=0;
    virtual void add(EntryProf&  , double x, double y, double vx)=0;
    virtual void add(EntryProf2D&, double x, double y, double vx, double vy)=0;
  };
};

#endif
