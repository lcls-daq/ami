#ifndef Pds_DescImage_HH
#define Pds_DescImage_HH

#include "ami/data/DescEntry.hh"

namespace Ami {

  class SubFrame {
  public:
    unsigned short x;
    unsigned short y;
    unsigned short nx;
    unsigned short ny;
  };

  class DescImage : public DescEntry {
  public:
    DescImage(const Pds::DetInfo& info,
	      unsigned channel,
	      const char* name,
	      unsigned nbinsx, unsigned nbinsy, 
	      int ppbx=1, int ppby=1); // pixels per bin

    DescImage(const char* name,
	      unsigned nbinsx, unsigned nbinsy, 
	      int ppbx=1, int ppby=1); // pixels per bin

    unsigned nbinsx() const;
    unsigned nbinsy() const;
    int ppxbin() const;
    int ppybin() const;
    float xlow() const;
    float xup() const;
    float ylow() const;
    float yup() const;
    int xbin(float) const;
    int ybin(float) const;
    float binx(unsigned) const;
    float biny(unsigned) const;

    void params(unsigned nbinsx,
		unsigned nbinsy,
		int ppxbin,
		int ppybin);

    void add_frame(unsigned x,
		   unsigned y,
		   unsigned nx,
		   unsigned ny);

    unsigned nframes() const;

    const SubFrame& frame(unsigned) const;

    bool xy_bounds(int& x0, int& x1, int& y0, int& y1) const;
    bool xy_bounds(int& x0, int& x1, int& y0, int& y1, unsigned frame) const;

    bool rphi_bounds(int& x0, int& x1, int& y0, int& y1,
		     double xc, double yc, double r) const;
    bool rphi_bounds(int& x0, int& x1, int& y0, int& y1,
		     double xc, double yc, double r, unsigned frame) const;

  private:
    unsigned short _nbinsx;
    unsigned short _nbinsy;
    unsigned short _ppbx;
    unsigned short _ppby;
    enum { MAX_SUBFRAMES=64 };
    unsigned _nsubframes;
    SubFrame _subframes[MAX_SUBFRAMES];
  };

  inline unsigned DescImage::nbinsx() const {return _nbinsx;}
  inline unsigned DescImage::nbinsy() const {return _nbinsy;}
  inline int DescImage::ppxbin() const {return _ppbx;}
  inline int DescImage::ppybin() const {return _ppby;}
  inline float DescImage::xlow() const {return 0;}
  inline float DescImage::xup() const {return _nbinsx*_ppbx;}
  inline float DescImage::ylow() const {return 0;}
  inline float DescImage::yup() const {return _nbinsy*_ppby;}
  inline int DescImage::xbin(float x) const {return int(x)/_ppbx;}
  inline int DescImage::ybin(float y) const {return int(y)/_ppby;}
  inline float DescImage::binx(unsigned b) const {return float(b)*_ppbx;}
  inline float DescImage::biny(unsigned b) const {return float(b)*_ppby;}
  inline unsigned DescImage::nframes() const { return _nsubframes; }
  inline const SubFrame& DescImage::frame(unsigned i) const { return _subframes[i]; }
};

#endif
