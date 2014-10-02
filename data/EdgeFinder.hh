#ifndef Ami_EdgeFinder_hh
#define Ami_EdgeFinder_hh

#include "ami/data/AbsOperator.hh"
#include "ami/data/VectorArray.hh"

namespace Ami {

  class EntryRef;
  class EntryWaveform;

  class EdgeFinderConfig {
  public:
    void load(const char*&);
    void save(char*&) const;
  public:
    double     _fraction;
    bool       _leading_edge;
    double     _deadtime;
    double     _threshold_value;
    double     _baseline_value;
  };

  class Edges : public VectorArray {
  public:
    enum Parameter { Ampl, Time, NumberOf };
    static const char* name(Parameter);
  public:
    Edges() : VectorArray(NumberOf) {}
    ~Edges() {}
  };

  //
  //  Edge finder
  //
  class EdgeFinder : public AbsOperator {
  public:
    EdgeFinder(const char*,
	       const EdgeFinderConfig&);
    EdgeFinder(const char*&);
    ~EdgeFinder();
  public:
    void use();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
    void       _invalid  ();
    void _hist_edge(double               peak, 
		    unsigned             start, 
		    double&              last,
		    const EntryWaveform& entry) const;
  private:
    enum { NAME_LEN = 32 };
    char             _name[NAME_LEN];

    EdgeFinderConfig _config;
    Edges            _output;
    EntryRef*        _entry;
    bool             _v;
    int              _index;
  };

};

#endif
