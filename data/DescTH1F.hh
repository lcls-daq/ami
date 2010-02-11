#ifndef Pds_ENTRYDESCTH1F_HH
#define Pds_ENTRYDESCTH1F_HH

#include "ami/data/DescEntry.hh"

namespace Ami {

  class DescTH1F : public DescEntry {
  public:
    DescTH1F(const char* name, 
	     const char* xtitle, 
	     const char* ytitle, 
	     unsigned nbins, 
	     float xlow, 
	     float xup,
	     bool normalize=true);

    void params(unsigned nbins, float xlow, float xup);
    void clear();

    unsigned nbins() const;
    float xlow() const;
    float xup() const;

  private:
    unsigned short _nbins;
    unsigned short _unused;
    float _xlow;
    float _xup;
  };

  inline unsigned DescTH1F::nbins() const {return _nbins;}
  inline float DescTH1F::xlow() const {return _xlow;}
  inline float DescTH1F::xup() const {return _xup;}
};

#endif
