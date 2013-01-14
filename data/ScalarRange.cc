#include "ami/data/ScalarRange.hh"

#include <limits>
#include <new>

using namespace Ami;

ScalarRange::~ScalarRange() {}

ScalarRange::ScalarRange()
{
  _y[3] = std::numeric_limits<double>::max();
  _y[4] = std::numeric_limits<double>::min();
}

void ScalarRange::setto(const ScalarRange& entry) 
{
  _y[0] = entry._y[0];
  _y[1] = entry._y[1];
  _y[2] = entry._y[2];
  _y[3] = entry._y[3];
  _y[4] = entry._y[4];
}

void ScalarRange::add(const ScalarRange& entry) 
{
  _y[0] += entry._y[0];
  _y[1] += entry._y[1];
  _y[2] += entry._y[2];
  if (entry._y[3] < _y[3])
    _y[3] = entry._y[3];
  if (entry._y[4] > _y[4])
    _y[4] = entry._y[4];
}

void ScalarRange::merge(char* p) const
{
  double* y = reinterpret_cast<double*>(p);
  y[0] += _y[0];
  y[1] += _y[1];
  y[2] += _y[2];
  if (_y[3] < y[3])
    y[3] = _y[3];
  if (_y[4] > y[4])
    y[4] = _y[4];
}

