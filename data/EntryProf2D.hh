#ifndef Pds_ENTRYPROF2D_HH
#define Pds_ENTRYPROF2D_HH

#include <stdio.h>
#include <math.h>

#include "ami/data/Entry.hh"
#include "ami/data/DescProf2D.hh"
#include "ndarray/ndarray.h"

namespace Ami {

  class EntryProf2D : public Entry {
  public:
    EntryProf2D(const Pds::DetInfo& info, unsigned channel, const char* name, const char* xtitle, const char* ytitle);
    EntryProf2D(const DescProf2D& desc);

    virtual ~EntryProf2D();

    void params(unsigned nxbins, float xlow, float xup,
                unsigned nybins, float ylow, float yup,
                const char* names);
    void params(const DescProf2D& desc);

    double zmean(unsigned xbin, unsigned ybin) const;
    double sigma(unsigned xbin, unsigned ybin) const;

    double zsum(unsigned xbin, unsigned ybin) const;
    void zsum(double z, unsigned xbin, unsigned ybin);

    double z2sum(unsigned xbin, unsigned ybin) const;
    void z2sum(double z2, unsigned xbin, unsigned ybin);

    double nentries(unsigned xbin, unsigned ybin) const;
    void nentries(double nent, unsigned xbin, unsigned ybin);

    const ndarray<double,2>& entries() const;
    const ndarray<double,2>& zsum   () const;
    const ndarray<double,2>& z2sum  () const;

    void addz(double z, unsigned xbin, unsigned ybin, double w=1);
    void addz(double z, double x, double y, double w=1);

    enum Info { XUnderflow, XOverflow, YUnderflow, YOverflow, Normalization, InfoSize };
    double info(Info) const;
    void   info(double, Info);
    void   addinfo(double, Info);

    void setto(const EntryProf2D& entry);
    void add  (const EntryProf2D&);
    void sum  (const EntryProf2D&, const EntryProf2D&);
    void diff (const EntryProf2D&, const EntryProf2D&);

    // Implements Entry
    virtual const DescProf2D& desc() const;
    virtual DescProf2D& desc();

  private:
    virtual void _merge(char*) const;

  private:
    void build(unsigned nxbins, unsigned nybins);

  private:
    DescProf2D _desc;

  private:
    ndarray<double,2> _zsum;
    ndarray<double,2> _z2sum;
    ndarray<double,2> _nentries;
  };

  inline double EntryProf2D::zmean(unsigned xbin, unsigned ybin) const 
  {
    double n = _nentries[ybin][xbin];
    if (n > 0) {
      double y = _zsum[ybin][xbin];
      double mean = y/n;
      return mean;
    } else {
      return 0;
    }
  }

  inline double EntryProf2D::sigma(unsigned xbin, unsigned ybin) const 
  {
    double n = _nentries[ybin][xbin];
    if (n > 0) {
      double y  = _zsum [ybin][xbin];
      double y2 = _z2sum[ybin][xbin];
      double mean = y/n;
      double s = sqrt(y2/n-mean*mean);
      return s;
    } else {
      return 0;
    }
  }

  inline double EntryProf2D::zsum(unsigned xbin, unsigned ybin) const {return _zsum[ybin][xbin];}
  inline   void EntryProf2D::zsum(double z, unsigned xbin, unsigned ybin) {_zsum[ybin][xbin] = z; }

  inline double EntryProf2D::z2sum(unsigned xbin, unsigned ybin) const {return _z2sum[ybin][xbin];}
  inline void EntryProf2D::z2sum(double z2, unsigned xbin, unsigned ybin) {_z2sum[ybin][xbin]=z2;}

  inline double EntryProf2D::nentries(unsigned xbin,unsigned ybin) const {return _nentries[ybin][xbin];}
  inline void EntryProf2D::nentries(double nent, unsigned xbin, unsigned ybin) {_nentries[ybin][xbin]=nent;}

  inline void EntryProf2D::addz(double z, unsigned xbin, unsigned ybin, double w) 
  {
    _zsum    [ybin][xbin] += z*w;
    _z2sum   [ybin][xbin] += z*z*w;
    _nentries[ybin][xbin] += w;
  }

  inline double EntryProf2D::info(Info i) const { return *(_nentries.end()+int(i)); }
  inline void   EntryProf2D::info(double y,Info i) { *(_nentries.end()+int(i)) = y; }
  inline void   EntryProf2D::addinfo(double y,Info i) { *(_nentries.end()+int(i)) += y; }

  inline const ndarray<double,2>& EntryProf2D::entries() const { return _nentries; }
  inline const ndarray<double,2>& EntryProf2D::zsum   () const { return _zsum; }
  inline const ndarray<double,2>& EntryProf2D::z2sum  () const { return _z2sum; }

};

#endif
