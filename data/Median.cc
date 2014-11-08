#include "ami/data/Median.hh"

using namespace Ami;

double Median::median() const
{
  double w = -0.5*_wgtsum;
  for(std::set<Entry>::const_iterator it=_set.begin();
      it!=_set.end(); it++)
    if ( (w+=it->_wgt) >= 0 )
      return it->_val;

  return _set.begin()->_val;
}
