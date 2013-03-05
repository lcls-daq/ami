#ifndef Ami_Cds_hh
#define Ami_Cds_hh

#include <sys/uio.h>

#include "ami/data/Desc.hh"
#include "ami/data/EntryList.hh"

#include <list>

namespace Ami {

  class Entry;
  class Semaphore;

  class Cds {
  public:
    Cds(const char* name);
    ~Cds();
  
    unsigned       add         (Entry* entry);
    void           add         (Entry* entry, unsigned signature);
    void           remove      (Entry* entry);
    const Entry*   entry       (int signature) const;
    Entry*         entry       (int signature);
    unsigned short totalentries() const { return _entries.size(); }
    unsigned short totaldescs  () const { return _entries.size()+1; }

    void reset      ();
    void sort       ();
    void showentries() const;

    //  serialize
    unsigned description() const { return totaldescs(); }
    unsigned payload    () const { return totalentries(); }

    void     description(iovec*) const;
    void     description(iovec*, EntryList) const;
    void     payload    (iovec*);
    unsigned payload    (iovec*, EntryList);
    void     invalidate_payload();
    void     mirror     (Cds&);

    enum ReqOpt { None, All };
    void      request    (ReqOpt);
    void      request    (const Entry&, bool);
    EntryList request    () const { return _request; }

    Semaphore& payload_sem() const { return *_payload_sem; }
  private:
    void adjust();

  private:
    typedef std::list<Entry*> EnList;
    Desc              _desc;
    EnList            _entries;
    mutable Semaphore* _payload_sem;
    unsigned          _signature;
    EntryList         _request;
  };
};

#endif
