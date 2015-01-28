#include "ami/data/VectorArray.hh"

using namespace Ami;

static const unsigned SizeStep = 50;

VectorArray::VectorArray(unsigned nelements) :
  _size    (0),
  _nentries(0),
  _elements(nelements)
{ resize(SizeStep); }

VectorArray::~VectorArray()
{ resize(0); }

void VectorArray::resize(unsigned size)
{
  if (size) {
    if (size < _size) 
      return;
    size += SizeStep;
  }

  if (_size)
    for(unsigned i=0; i<_elements.size(); i++)
      delete[] _elements[i];

  _size = size;

  if (size) 
    for(unsigned i=0; i<_elements.size(); i++)
      _elements[i] = new double[size];
}

void VectorArray::reset() { _nentries=0; }

void VectorArray::append(const double* v)
{
  for(unsigned i=0; i<_elements.size(); i++)
    _elements[i][_nentries] = v[i];
  _nentries++;
}
