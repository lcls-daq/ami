#ifndef Pds_DESCPROF2D_HH
#define Pds_DESCPROF2D_HH

#include "ami/data/DescEntryW.hh"

namespace Ami {

  class DescProf2D : public DescEntryW {
  public:
    DescProf2D(const char* name, const char* xtitle, const char* ytitle, 
               unsigned nxbins, float xlow, float xup, 
               unsigned nybins, float ylow, float yup, 
               const char* names,
	       Stat        stat=Mean,
               const char* weight="");

    DescProf2D(const Pds::DetInfo& info,
               unsigned channel,
               const char* name, const char* xtitle, const char* ytitle, 
               unsigned nxbins, float xlow, float xup, 
               unsigned nybins, float ylow, float yup, 
               const char* names, Stat stat=Mean);

    //  Special constructor for centering outer bins
    //    on xlow,xup.
    enum InitOpt { Inclusive };
    DescProf2D(InitOpt,
               const char* name, const char* xtitle, const char* ytitle, 
               unsigned nxbins, float xlow, float xup, 
               unsigned nybins, float ylow, float yup, 
               const char* names,
	       Stat        stat=Mean,
               const char* weight="");


    unsigned nxbins() const;
    float xlow() const;
    float xup() const;
    unsigned xbin(float x) const;

    unsigned nybins() const;
    float ylow() const;
    float yup() const;
    unsigned ybin(float y) const;

    const char* names() const;  

    void params(unsigned nxbins, float xlow, float xup,
                unsigned nybins, float ylow, float yup,
                const char* names);
  private:
    void _setup_bins(unsigned, float, float, float&, float&);

  private:
    enum {NamesSize=256};
    char _names[NamesSize];
    uint32_t _nxbins;
    float _xlow;
    float _xup;
    uint32_t _nybins;
    float _ylow;
    float _yup;
  };

  inline unsigned DescProf2D::nxbins() const {return _nxbins;}
  inline float DescProf2D::xlow() const {return _xlow;}
  inline float DescProf2D::xup() const {return _xup;}
  inline unsigned DescProf2D::xbin(float x) const {return unsigned((x-_xlow)*_nxbins/(_xup-_xlow));}

  inline unsigned DescProf2D::nybins() const {return _nybins;}
  inline float DescProf2D::ylow() const {return _ylow;}
  inline float DescProf2D::yup() const {return _yup;}
  inline unsigned DescProf2D::ybin(float y) const {return unsigned((y-_ylow)*_nybins/(_yup-_ylow));}

  inline const char* DescProf2D::names() const {return _names;}
};

#endif
