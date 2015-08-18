#ifndef Ami_XtcCache_hh
#define Ami_XtcCache_hh

#include <boost/shared_ptr.hpp>

#include <map>

namespace Pds { class Xtc; }

namespace Ami {
  class XtcCache {
  public:
    XtcCache();
    ~XtcCache();
  public:
    boost::shared_ptr<Pds::Xtc> cache(Pds::Xtc*);
  private:
    std::map<Pds::Xtc*,boost::shared_ptr<Pds::Xtc> > _umap;
  };
};

#endif
