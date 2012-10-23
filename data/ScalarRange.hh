#ifndef Ami_ScalarRange_HH
#define Ami_ScalarRange_HH

#include <math.h>

namespace Ami {

  class ScalarRange {
  public:
    ScalarRange();
    ~ScalarRange();

    //  The contents are organized as 
    double entries() const;
    double sum    () const;
    double sqsum  () const;
    double mean   () const;
    double rms    () const;
    double min    () const;
    double max    () const;
    void   addcontent(double y);

    void   setto(const ScalarRange&);
    void   add  (const ScalarRange&);
  private:
    double _y[5];
  };

  inline double ScalarRange::entries() const { return _y[0]; }
  inline double ScalarRange::sum    () const { return _y[1]; }
  inline double ScalarRange::sqsum  () const { return _y[2]; }
  inline double ScalarRange::mean   () const { return _y[0] ? _y[1]/_y[0] : 0; }
  inline double ScalarRange::rms    () const { return _y[0] ? sqrt((_y[2] - _y[1]*_y[1]/_y[0])/_y[0]) : 0; }
  inline double ScalarRange::min    () const { return _y[3]; }
  inline double ScalarRange::max    () const { return _y[4]; }
  inline void   ScalarRange::addcontent(double y) { _y[0]++; _y[1]+=y; _y[2]+=y*y; if (y<_y[3]) _y[3]=y; if (y>_y[4]) _y[4]=y; }
};

#endif
