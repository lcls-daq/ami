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
    VAPlot(int ix, int iy, int iz,
	   int               accumulate,
	   const  DescImage& output);
    VAPlot(const char*&);
    VAPlot(const char*&, const DescEntry&);
    ~VAPlot();
  public:
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
  private:
    int               _ix;
    int               _iy;
    int               _iz;
    int               _accumulate;   /// single event (-1), running sum(0), fixed event sum(>0)
    char              _desc[sizeof(DescImage)];
    mutable int       _current;
    EntryImage*       _output_entry; /// Holds single event result and indefinite running sum
    EntryImage*       _cache;        /// Holds fixed event number sum
  };

};

#endif