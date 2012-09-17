#ifndef Ami_Zoom_hh
#define Ami_Zoom_hh

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class EntryImage;

  class Zoom : public AbsOperator {
  public:
    Zoom(const DescEntry&, const AbsOperator& o);
    Zoom(const char*&, const DescEntry&);
    ~Zoom();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
  private:
    EntryImage*         _entry;
  };

};

#endif
