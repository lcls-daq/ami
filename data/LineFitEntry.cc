#include "ami/data/LineFitEntry.hh"
#include "ami/data/LeastSquaresFit.hh"
#include "ami/data/MedianSlopeFit.hh"

using namespace Ami;

LineFitEntry* LineFitEntry::instance(LineFit::Method m)
{
  switch(m) {
  case LineFit::LeastSquares: return new LeastSquaresFit;
  case LineFit::MedianSlope : return new MedianSlopeFit;
  default: abort(); 
  }
}
