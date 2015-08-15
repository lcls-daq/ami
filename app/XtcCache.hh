#ifndef Ami_XtcCache_hh
#define Ami_XtcCache_hh

#include <boost/shared_ptr.hpp>

namespace Pds { class Xtc; }

namespace Ami {
  class XtcCache {
  public:
    XtcCache();
    ~XtcCache();
  public:
    boost::shared_ptr<Xtc> cache(Xtc*);
  private:
    std::map<Xtc*,boost::shared_ptr<Xtc> > _umap;
  };
};

#endif
