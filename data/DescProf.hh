#ifndef Pds_DESCPROF_HH
#define Pds_DESCPROF_HH

#include "ami/data/DescEntry.hh"

namespace Ami {

  class DescProf : public DescEntry {
  public:
    DescProf(const char* name, const char* xtitle, const char* ytitle, 
		unsigned nbins, float xlow, float xup, const char* names);

    unsigned nbins() const;
    float xlow() const;
    float xup() const;
    const char* names() const;  

    void params(unsigned nbins, float xlow, float xup, const char* names);

  private:
    enum {NamesSize=256};
    char _names[NamesSize];
    unsigned short _nbins;
    unsigned short _unused;
    float _xlow;
    float _xup;
  };

  inline unsigned DescProf::nbins() const {return _nbins;}
  inline float DescProf::xlow() const {return _xlow;}
  inline float DescProf::xup() const {return _xup;}
  inline const char* DescProf::names() const {return _names;}
};

#endif
