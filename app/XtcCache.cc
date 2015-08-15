#include "ami/app/XtcCache.hh"

#include "pdsdata/compress/CompressedXtc.hh"
#include "pdsdata/xtc/Xtc.hh"

using namespace Ami;

static void Destroy(Xtc*) {}


XtcCache::XtcCache() {}

XtcCache::~XtcCache() {}

boost::shared_ptr<Xtc> cache(Xtc* xtc)
{
  boost::shared_ptr<Xtc> pxtc;
  if (!xtc->contains.compressed())
    pxtc = boost::shared_ptr<Xtc>(xtc,Destroy);
  else {
    std::map<Xtc*,boost::shared_ptr<Xtc>::iterator it=_umap.find(xtc);
    if (it!=_umap.end())
      pxtc = it->second;
    else
      pxtc = _umap[xtc] = Pds::CompressedXtc::uncompress(*xtc);
  }            
}
