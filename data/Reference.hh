#ifndef Ami_Reference_hh
#define Ami_Reference_hh

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;

  /**
   *   An operator to add a previously saved reference event to the data stream
   *   for overlay or EntryMath calculations.
   */
  class Reference : public AbsOperator {
  public:
    Reference(const char* path);
    Reference(const char*&, const DescEntry&);
    ~Reference();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
    void       _invalid  () {}
  private:
    enum { PATHLEN=256 };
    char           _path[PATHLEN];
    char           _buffer[4096];
    Entry*         _entry;
  };

};

#endif
