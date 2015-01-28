#ifndef Ami_VAPlot_hh
#define Ami_VAPlot_hh

#include "ami/data/AbsOperator.hh"
#include "ami/data/DescImage.hh"

namespace Ami {

  class Cds;
  class DescEntry;
  class EntryImage;

  //
  //  Operator to map vector arrays onto an image
  //
  class VAPlot : public AbsOperator {
  public:
    enum Option { Single, AutoRefresh };
    VAPlot(int ix, int iy, int iz,
	   Option,
	   const  DescImage& output);
    VAPlot(int ix, int iy, int iz,
	   int               accumulate,
	   const  DescImage& output);
    VAPlot(const char*&);
    VAPlot(const char*&, const DescEntry&);
    ~VAPlot();
  public:
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
    void       _invalid  ();
  private:
    int               _ix;
    int               _iy;
    int               _iz;
    int               _accumulate;   /// auto refresh (-2), single event (-1), running sum(0), fixed event sum(>0)
    uint32_t          _desc[sizeof(DescImage)/sizeof(uint32_t)];
    mutable int       _current;
    EntryImage*       _output_entry; /// Holds single event result and indefinite running sum
    EntryImage*       _cache;        /// Holds fixed event number sum
  };

};

#endif
