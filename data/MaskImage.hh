#ifndef Ami_MaskImage_hh
#define Ami_MaskImage_hh

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;

  class MaskImage : public AbsOperator {
  public:
    MaskImage(const char*);
    MaskImage(const char*&, const DescEntry&);
    ~MaskImage();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
  private:
    enum { PATH_LEN=256 };
    char           _path[PATH_LEN];
    Entry*         _output;
  };

};

#endif
