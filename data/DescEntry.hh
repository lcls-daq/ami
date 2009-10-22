#ifndef Pds_DESCENTRY_HH
#define Pds_DESCENTRY_HH

#include "ami/data/Desc.hh"

namespace Ami {

  class DescEntry : public Desc {
  public:
    enum Type {Scalar, TH1F, TH2F, Prof, Image, Waveform, Scan};
    Type type() const;

    const char* xtitle() const;
    const char* ytitle() const;
    unsigned short size() const;

    bool isnormalized() const;

    void xwarnings(float warn, float err);
    bool xhaswarnings() const;
    float xwarn() const;
    float xerr() const;

    void ywarnings(float warn, float err);
    bool yhaswarnings() const;
    float ywarn() const;
    float yerr() const;

  protected:
    DescEntry(const char* name, const char* xtitle, const char* ytitle, 
	      Type type, unsigned short size, bool isnormalized=true);

  private:
    enum {TitleSize=128};
    char _xtitle[TitleSize];
    char _ytitle[TitleSize];
    int short _group;
    int short _options;
    unsigned short _type;
    unsigned short _size;
    float _xwarn;
    float _xerr;
    float _ywarn;
    float _yerr;

  private:
    enum Option {Normalized, XWarnings, YWarnings};
  };
};

#endif
