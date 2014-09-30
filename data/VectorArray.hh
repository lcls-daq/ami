#ifndef Ami_VectorArray_hh
#define Ami_VectorArray_hh

#include <vector>
#include <stdint.h>

namespace Ami {
  class VectorArray {
  public:
    VectorArray(unsigned nelements);
    ~VectorArray();
  public:
    unsigned nelements() const { return _elements.size(); }
    unsigned size     () const { return _size; }
    unsigned nentries () const { return _nentries; }
    const double* element(unsigned i) const { return _elements[i]; }
  public:
    void resize(unsigned size);
    void reset();
    void append(const double*);
  private:
    uint32_t _size;
    uint32_t _nentries;
    std::vector<double*> _elements;
  };
};

#endif
