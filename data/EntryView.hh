#ifndef Pds_EntryView_HH
#define Pds_EntryView_HH

//
//  This class allows a read-only view of an Entry
//

#include "ami/data/Entry.hh"

namespace Ami {

  class EntryView : public Entry {
  public:
    EntryView(const Entry& b) : 
      _base (b) 
    {
      _payloadsize = b._payloadsize;
      _payload     = b._payload;
    }
    ~EntryView() { _payload = 0; }

    DescEntry& desc()             { return const_cast<Entry*>(&_base)->desc(); }
    const DescEntry& desc() const { return _base.desc(); }

  private:
    const Entry& _base;
  };
};

#endif

  
