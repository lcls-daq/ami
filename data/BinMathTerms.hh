#ifndef Ami_BinMathTerms_hh
#define Ami_BinMathTerms_hh

#include "ami/data/Expression.hh"

namespace Ami {
  enum Moment { None, Zero, First, Second, 
                Contrast, XCenterOfMass, YCenterOfMass,
                Mean, Variance };
  class Entry;
  namespace BinMathC {

    class EntryWaveformTerm : public Term {
    public:
      EntryWaveformTerm(const Entry*& e, unsigned lo, unsigned hi,
                        Moment mom);
      ~EntryWaveformTerm() {}
    public:
      double evaluate() const;
    private:
      const Entry*& _entry;
      unsigned _lo, _hi;
      Moment _mom;
    };

    class EntryTH1FTerm : public Term {
    public:
      EntryTH1FTerm(const Entry*& e, unsigned lo, unsigned hi,
                    Moment mom);
      ~EntryTH1FTerm() {}
    public:
      double evaluate() const;
    private:
      const Entry*& _entry;
      unsigned _lo, _hi;
      Moment _mom;
    };

    class EntryProfTerm  : public Term {
    public:
      EntryProfTerm(const Entry*& e, unsigned lo, unsigned hi,
                    Moment mom);
      ~EntryProfTerm() {}
    public:
      double evaluate() const;
    private:
      const Entry*& _entry;
      unsigned _lo, _hi;
      Moment _mom;
    };
    
    class EntryImageTerm : public Term {
    public:
      EntryImageTerm(const Entry*& e, 
                     unsigned xlo, unsigned xhi, 
                     unsigned ylo, unsigned yhi,
                     Moment mom);
      ~EntryImageTerm() {}
    public:
      double evaluate() const;
    private:
      const Entry*& _entry;
      unsigned _xlo, _xhi, _ylo, _yhi;
      Moment _mom;
    };

    class EntryImageTermF : public Term {
    public:
      EntryImageTermF(const Entry*& e, 
                      double xc, double yc, 
                      double r0, double r1, 
                      double f0, double f1,
                      Moment mom);
      ~EntryImageTermF() {}
    public:
      double evaluate() const;
    private:
      const Entry*& _entry;
      double _xc, _yc, _r0, _r1, _f0, _f1;
      Moment _mom;
    };
  };
};
#endif
