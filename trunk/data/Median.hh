#ifndef Ami_MedianEntry_hh
#define Ami_MedianEntry_hh

#include <set>

namespace Ami {
  class Median {
  public:
    Median() : _wgtsum(0) {}
    ~Median() {}
  public:
    void   accum (double value, double weight=1) {
      std::pair< std::set<Entry>::iterator,bool > r = 
	_set.insert(Entry(value,weight));
      if (!r.second) r.first->_wgt += weight;
      _wgtsum += weight;
    }
  public:
    double median() const;
    double weight() const { return _wgtsum; }
  private:

    class Entry {
    public:
      Entry(double val, 
	    double wgt) : _val(val), _wgt(wgt) {}
      ~Entry() {}
    public:
      bool operator<(const Entry& o) const { return _val < o._val; }
    private:
      double _val;
      mutable double _wgt;
      friend class Median;
    };

    std::set< Entry > _set;
    double _wgtsum;
  };
};

#endif
