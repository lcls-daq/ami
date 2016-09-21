#ifndef Pds_DescImage_HH
#define Pds_DescImage_HH

#include "ami/data/DescEntry.hh"

namespace Ami {

  enum Rotation { D0, D90, D180, D270, NPHI=4 };

  class SubFrame {
  public:
    SubFrame() : x(0), y(0), nx(0), ny(0), r(D0) {}
  public:
    uint16_t x;  // units are bins
    uint16_t y;
    uint16_t nx;
    uint16_t ny;
    Rotation r;  // rotation from standard orientation
  };

  class ImageMask;

  class DescImage : public DescEntry {
  public:
    DescImage(const Pds::DetInfo& info,
	      const char* name,
              const char* zunits,
	      unsigned nbinsx, unsigned nbinsy, 
	      int ppbx=1, int ppby=1,  // pixels per bin
              bool pedCalib=false, bool gainCalib=false, bool rmsCalib=false);

    DescImage(const Pds::DetInfo& info,
	      unsigned channel,
	      const char* name,
	      unsigned nbinsx, unsigned nbinsy, 
	      int ppbx=1, int ppby=1,  // detector pixels per bin
	      int dpbx=1, int dpby=1,  // display pixels per bin
              unsigned xp0=0, unsigned yp0=0);

    DescImage(const char* name,
	      unsigned nbinsx, unsigned nbinsy, 
	      int ppbx=1, int ppby=1,  // pixels per bin
              unsigned xp0=0, unsigned yp0=0,   // pixel origin
              bool isnormalized=true);

    DescImage(const DescImage&);

    ///  Adds (replaces) a mask to the image
    DescImage(const DescImage&, const char*);

    ~DescImage();

    unsigned nbinsx() const;
    unsigned nbinsy() const;
    int ppxbin() const;
    int ppybin() const;
    float xlow() const; // Inclusive bound
    float xup() const;  // Exclusive bound
    float ylow() const; // Inclusive bound
    float yup() const;  // Exclusive bound
    int xbin(float) const;
    int ybin(float) const;
    float binx(unsigned) const;
    float biny(unsigned) const;

    float mmppx() const { return _mmppx; }
    float mmppy() const { return _mmppy; }

    unsigned disppbx() const;
    unsigned disppby() const;
    unsigned ndispx() const;
    unsigned ndispy() const;

    void params(unsigned nbinsx,
		unsigned nbinsy,
		int ppxbin,
		int ppybin);

    void set_scale(float mmppx,
		   float mmppy);

    void add_frame(unsigned x,  // units are bins
		   unsigned y,
		   unsigned nx,
		   unsigned ny,
                   Rotation r=D0);

    void set_mask(const ImageMask&);

    unsigned nframes() const;

    const SubFrame& frame(unsigned) const;

    const ImageMask* mask() const;

    //  Find inclusive bounds
    bool xy_bounds(int& x0, int& x1, int& y0, int& y1) const;
    bool xy_bounds(int& x0, int& x1, int& y0, int& y1, unsigned frame) const;

    bool rphi_bounds(int& x0, int& x1, int& y0, int& y1,
		     double xc, double yc, double r) const;
    bool rphi_bounds(int& x0, int& x1, int& y0, int& y1,
		     double xc, double yc, double r, unsigned frame) const;

    static void deserialize(void*, const char*&);
  private:
    uint16_t _nbinsx;
    uint16_t _nbinsy;
    ///  Detector pixels per bin
    uint8_t  _ppbx;
    uint8_t  _ppby;
    ///  Display pixels per bin
    uint8_t  _dpbx;
    uint8_t  _dpby;
    ///  Detector pixel coordinates at bin origin
    int32_t  _xp0;
    int32_t  _yp0;
    float    _mmppx;
    float    _mmppy;
    uint32_t _reserved;
    enum { MAX_SUBFRAMES=64 };
    uint32_t _nsubframes;
    SubFrame _subframes[MAX_SUBFRAMES];
    enum { PATHLEN=256 };
    char       _mask_path[PATHLEN];
    union {
      ImageMask* _ptr;
      uint64_t   _field;
    } _mask;

    friend class RotateImage;
  };

  inline unsigned DescImage::nbinsx() const {return _nbinsx;}
  inline unsigned DescImage::nbinsy() const {return _nbinsy;}
  inline int DescImage::ppxbin() const {return _ppbx;}
  inline int DescImage::ppybin() const {return _ppby;}
  inline float DescImage::xlow() const {return _xp0;}
  inline float DescImage::xup () const {return _xp0+_nbinsx*_ppbx;}
  inline float DescImage::ylow() const {return _yp0;}
  inline float DescImage::yup () const {return _yp0+_nbinsy*_ppby;}
  inline int DescImage::xbin(float x) const {return (int(x)-_xp0)/_ppbx;}
  inline int DescImage::ybin(float y) const {return (int(y)-_yp0)/_ppby;}
  inline float DescImage::binx(unsigned b) const {return _xp0+float(b)*_ppbx;}
  inline float DescImage::biny(unsigned b) const {return _yp0+float(b)*_ppby;}
  inline unsigned DescImage::nframes() const { return _nsubframes; }
  inline const SubFrame& DescImage::frame(unsigned i) const { return _subframes[i]; }
  inline const ImageMask* DescImage::mask() const { return _mask._ptr; }

  inline unsigned DescImage::ndispx() const {return _nbinsx*_dpbx;}
  inline unsigned DescImage::ndispy() const {return _nbinsy*_dpby;}
  inline unsigned DescImage::disppbx() const { return _dpbx; }
  inline unsigned DescImage::disppby() const { return _dpby; }
};

#endif
