#ifndef Pds_DescImage_HH
#define Pds_DescImage_HH

#include "ami/data/DescEntry.hh"

namespace Ami {

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

    void params(unsigned nbinsx,
		unsigned nbinsy,
		int ppxbin,
		int ppybin);

  private:
    unsigned short _nbinsx;
    unsigned short _nbinsy;
    unsigned short _ppbx;
    unsigned short _ppby;
  };

  inline unsigned DescImage::nbinsx() const {return _nbinsx;}
  inline unsigned DescImage::nbinsy() const {return _nbinsy;}
  inline int DescImage::ppxbin() const {return _ppbx;}
  inline int DescImage::ppybin() const {return _ppby;}
  inline float DescImage::xlow() const {return 0;}
  inline float DescImage::xup() const {return _nbinsx*_ppbx;}
  inline float DescImage::ylow() const {return 0;}
  inline float DescImage::yup() const {return _nbinsy*_ppby;}
};

#endif
