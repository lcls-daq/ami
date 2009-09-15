#include "ami/data/DescImage.hh"

using namespace Ami;

DescImage::DescImage(const char* name, 
			   unsigned nbinsx, 
			   unsigned nbinsy, 
			   int ppbx,
			   int ppby) :
  DescEntry(name, "x", "y", Image, sizeof(DescImage)),
  _nbinsx(nbinsx ? nbinsx : 1),
  _nbinsy(nbinsy ? nbinsy : 1),
  _ppbx  (ppbx),
  _ppby  (ppby)
{}

void DescImage::params(unsigned nbinsx,
			  unsigned nbinsy,
			  int ppxbin,
			  int ppybin)
{
  _nbinsx = nbinsx ? nbinsx : 1;
  _nbinsy = nbinsy ? nbinsy : 1;
  _ppbx = ppxbin;
  _ppby = ppybin;
}
