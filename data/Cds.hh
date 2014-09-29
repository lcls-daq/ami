#ifndef Ami_Cds_hh
#define Ami_Cds_hh

#include <sys/uio.h>

#include "ami/data/Desc.hh"
#include "ami/data/EntryList.hh"

#include <list>
#include <string>

namespace Ami {

  class DescEntry;
  class Entry;
  class DataLock;
  class Cdu;

  class Cds {
  public:
    Cds(const char* name);
    ~Cds();
  
    unsigned       add         (Entry* entry);
    void           add         (Entry* entry, unsigned signature);
    void           remove      (Entry* entry);
    const Entry*   entry       (int signature) const;
    Entry*         entry       (int signature);
    const DescEntry* entry     (const char*) const;
    unsigned short totalentries() const { return _entries.size(); }
    unsigned short totaldescs  () const { return _entries.size()+1; }

    void reset      ();
    void sort       ();
    void showentries() const;
    std::string dump() const;

    //  serialize
    unsigned description() const { return totaldescs(); }
    unsigned payload    () const { return totalentries(); }

    void     description(iovec*) const;
    void     description(iovec*, EntryList) const;
    void     payload    (iovec*);
    unsigned payload    (iovec*, EntryList);
    void     invalidate_payload();
    void     mirror     (Cds&);
    void     refresh    ();

    enum ReqOpt { None, All };
    void      request    (ReqOpt);
    void      request    (const Entry&, bool);
    EntryList request    () const { return _request; }

    void      clear_used();
    void      reset_plots();

    DataLock& lock() const { return *_lock; }
    void      subscribe  (Cdu&);
    void      unsubscribe(Cdu&);
    
  private:
    void adjust();

  private:
    typedef std::list<Entry*> EnList;
    Desc              _desc;
    EnList            _entries;
    DataLock*         _lock;
    unsigned          _signature;
    EntryList         _request;
    typedef std::list<Cdu*> UList;
    UList             _users;
  };
};

#endif
