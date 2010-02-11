#include "ami/data/DescScan.hh"

using namespace Ami;

DescScan::DescScan(const char* name, 
		   const char* xtitle, 
		   const char* ytitle, 
		   unsigned nbins) :
  DescEntry( name, xtitle, ytitle, Scan, sizeof(DescScan)),
  _nbins(nbins)
{
}

DescScan::DescScan(const Pds::DetInfo& info,
		   unsigned channel,
		   const char* name, 
		   const char* xtitle, 
		   const char* ytitle, 
		   unsigned nbins) :
  DescEntry(info, channel, name, xtitle, ytitle, Scan, sizeof(DescScan)),
  _nbins(nbins)
{
}

void DescScan::params(unsigned nbins)
{
  _nbins = nbins;
}
