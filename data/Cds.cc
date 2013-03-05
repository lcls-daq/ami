#include <string.h>

#include "Cds.hh"
#include "Entry.hh"
#include "EntryView.hh"
#include "DescEntry.hh"
#include "ami/service/Semaphore.hh"

//#define DBUG

using namespace Ami;

Cds::Cds(const char* name) :
  _desc       (name),
  _payload_sem(new Semaphore(Semaphore::FULL)),
  _signature  (0)
{
  _desc.signature(_signature++);
}

Cds::~Cds()
{
  reset();
}

unsigned Cds::add(Entry* entry) 
{
  unsigned s = _signature++; 
  add(entry,s);
  return s; 
}

void Cds::add(Entry* entry, unsigned signature)
{
  entry->desc().signature(signature);
  _entries.push_back(entry);

#ifdef DBUG
  printf("Cds %s added entry %s (%p) type %d signature %d nentries %d\n",
   	 _desc.name(),entry->desc().name(),entry,entry->desc().type(),signature,totalentries());
#endif
  _request.insert(_entries.size()-1);
}

void Cds::remove(Entry* entry)
{
  if (!entry)
    return;

  _entries.remove(entry);

#ifdef DBUG
  printf("Cds %s removed entry %s (%p) type %d signature %d nentries %d\n",
  	 _desc.name(),entry->desc().name(),entry,entry->desc().type(),entry->desc().signature(),totalentries());
#endif
}

void Cds::reset()
{
  for (EnList::iterator it=_entries.begin(); it!=_entries.end(); it++) {
    Entry* entry = *it;

#ifdef DBUG    
    printf("Cds %s clearing entry %s type %d signature %d\n",
     	   _desc.name(),entry->desc().name(),entry->desc().type(),entry->desc().signature());
#endif
    delete entry;
  }
  _entries.clear();
  _request.fill(_entries.size());
}

const Entry*   Cds::entry       (int signature) const
{
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    const Entry* en = *it;
    if (en->desc().signature()==signature)
      return en;
  }
  return 0;
}

Entry*         Cds::entry       (int signature)
{
  for (EnList::iterator it=_entries.begin(); it!=_entries.end(); it++) {
    Entry* en = *it;
    if (en->desc().signature()==signature)
      return en;
  }
  return 0;
}

#include <stdio.h>
#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void Cds::showentries() const
{
  printf("%s serving %d entries:\n", _desc.name(), totalentries());
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    const Entry* en = *it;
    printf("  [%2d] %s\n", en->desc().signature(), en->desc().name());
  }
}

void Cds::description(iovec* iov) const
{
  Cds* cthis = const_cast<Cds*>(this);
  cthis->_desc.nentries(totalentries());
  iov->iov_base = (void*)&cthis->_desc;
  iov->iov_len  = sizeof(Desc);
  iov++;
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++, iov++) {
    const Entry* en = *it;
    iov->iov_base = (void*)&en->desc();
    iov->iov_len = en->desc().size();
  }
}

void Cds::description(iovec* iov, EntryList request) const
{
  Cds* cthis = const_cast<Cds*>(this);
  cthis->_desc.nentries(totalentries());
  iov->iov_base = (void*)&cthis->_desc;
  iov->iov_len  = sizeof(Desc);
  iov++;
  unsigned i=0;
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++, i++) {
    if (request.contains(i)) {
      const Entry* en = *it;
      iov->iov_base = (void*)&en->desc();
      iov->iov_len = en->desc().size();
      iov++;
    }
  }
}

void Cds::payload(iovec* iov)
{
  _request.fill(_entries.size());
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++, iov++)
    (*it)->payload(*iov);
}

unsigned Cds::payload(iovec* iov, EntryList request)
{
  _request = request;
  unsigned i=0;
  iovec* iov_b(iov);
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++,i++)
    if (request.contains(i))
      (*it)->payload(*iov++);

#ifdef DEBUG
  { unsigned psize(0);
    for(iovec* b=iov_b; b<iov; b++)
      psize += b->iov_len;
    printf("psize %x\n",psize); }
#endif
  return iov-iov_b;
}

void Cds::invalidate_payload()
{
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++)
    (*it)->invalid();
}

void Cds::request(const Entry& entry, bool r)
{
  unsigned b=0;
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++, b++)
    if ((*it)==&entry) {
      if (r)
        _request.insert(b);
      else
        _request.remove(b);
      break;
    }
}

void Cds::request(ReqOpt o)
{
  _request = EntryList(o==None ? EntryList::Empty : EntryList::Full);
}

void Cds::mirror(Cds& o)
{
  o.reset();
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    const Entry* en = *it;
    o.add(new EntryView(*en));
  }
}

static bool entry_comp(Entry* a, Entry* b)
{
  if (a->desc().info().phy() != b->desc().info().phy())
    return a->desc().info().phy() < b->desc().info().phy();
  if (a->desc().channel() != b->desc().channel())
    return a->desc().channel() < b->desc().channel();
  return strcmp(a->desc().name(),b->desc().name());
}

void Cds::sort()
{
  _entries.sort(entry_comp);
  for (EnList::iterator it=_entries.begin(); it!=_entries.end(); it++) {
    Entry* entry = *it;
    entry->desc().signature(_signature++);
  }
  _request.fill(_entries.size());
}
