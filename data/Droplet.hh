#ifndef Ami_Droplet_hh
#define Ami_Droplet_hh

/**
 **  Generate a vector array of hits (photons) for the indicated
 **  criteria set.  The vector is of size nphotons (per event), 
 **  and the array is fixed in size to the number of analysed
 **  parameters per photon.  Plots are generated from the resulting
 **  vector array, including an optional hit map.
 **/

#include "ami/data/AbsOperator.hh"
#include "ami/data/VectorArray.hh"

namespace Ami {

  class EntryRef;
  class FeatureCache;

  class DropletConfig {
  public:
    void load(const char*&);
    void save(char*&) const;
  public:
    unsigned seed_threshold;
    unsigned nbor_threshold;
    unsigned esum_min;
    unsigned esum_max;
    unsigned npix_min;
    unsigned npix_max;
  };

  class Droplets : public VectorArray {
  public:
    enum Parameter { Esum, X, Y, Np, Xmom, Ymom, R2mom, NumberOf };
    static const char* name(Parameter);
  public:
    Droplets() : VectorArray(NumberOf) {}
    ~Droplets() {}
  };

  //
  //  Photon finder
  //
  class Droplet : public AbsOperator {
  public:
    enum Option { Diagnostics };
  public:
    //    Droplet(Option);
    Droplet(const char*,
	    const DropletConfig&);
    Droplet(const char*&,
	    FeatureCache&);
    ~Droplet();
  public:
    void use();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate   (const Entry&) const;
    void*      _serialize (void*) const;
    bool       _valid     () const { return _v; }
    void       _invalid   ();
    bool       _accumulate(double,double);
  private:
    enum { NAME_LEN = 32 };
    char             _name[NAME_LEN];

    DropletConfig    _config;
    Droplets         _output;
    EntryRef*        _entry;
    bool             _v;
    FeatureCache*    _cache;
    int              _index;
  };
};

#endif
