#include <string.h>

#include "Cds.hh"
#include "Cdu.hh"
#include "Entry.hh"
#include "EntryView.hh"
#include "DescEntry.hh"
#include "ami/service/DataLock.hh"

#include <sstream>

//#define DBUG

using namespace Ami;

Cds::Cds(const char* name) :
  _desc       (name),
  _lock       (new DataLock),
  _signature  (0)
{
  _desc.signature(_signature++);
}

Cds::~Cds()
{
  reset();
  delete _lock;
}

unsigned Cds::add(Entry* entry) 
{
  unsigned s = _signature++; 
  add(entry,s);
  return s; 
}

void Cds::add(Entry* entry, unsigned signature)
{
  for (EnList::iterator it=_entries.begin(); it!=_entries.end(); it++) {
    Entry* e = *it;
    if (e->desc().signature() == int(signature)) {
      printf("Cds::add entry %p already exists with signature %d [%p]\n",
	     e, signature, entry);
      abort();
    }
  }

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
  for (UList::iterator it=_users.begin(); it!=_users.end(); it++)
    (*it)->clear_payload();
  _users.clear();

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

const DescEntry* Cds::entry       (const char* name) const
{
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    const Entry* en = *it;
    if (strcmp(en->desc().name(),name)==0)
      return &en->desc();
  }
  return 0;
}

void Cds::showentries() const
{
  printf("%s serving %d entries:\n", _desc.name(), totalentries());
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    const Entry* en = *it;
#ifdef DBUG
    printf("  [%2d] [%s] %s  agg %c  norm %c  cnt %c\n", 
           en->desc().signature(), 
	   DescEntry::type_str(en->desc().type()), 
           en->desc().name(),
           en->desc().aggregate   ()?'t':'f',
           en->desc().isnormalized()?'t':'f',
           en->desc().countmode   ()?'t':'f');
#else    
    printf("  [%2d] [%s] %s\n", 
	   en->desc().signature(), 
	   DescEntry::type_str(en->desc().type()), 
	   en->desc().name());
#endif
  }
}

std::string Cds::dump() const
{
  std::ostringstream s;
  s << _desc.name() << " serving " << totalentries() << " entries." << std::endl;
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    const Entry* en = *it;
#ifdef DBUG
    s << "  [" << en->desc().signature() 
      << "] [" << DescEntry::type_str(en->desc().type())
      << "] "  << en->desc().name() 
      << "  agg "  << (en->desc().aggregate   ()?'t':'f')
      << "  norm " << (en->desc().isnormalized()?'t':'f')
      << "  cnt "  << (en->desc().countmode()?'t':'f')
      << std::endl;
#else    
    s << "  [" << en->desc().signature() 
      << "] [" << DescEntry::type_str(en->desc().type())
      << "] "  << en->desc().name()
      << std::endl;
#endif
  }
  return s.str();
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

void Cds::refresh()
{
  unsigned i=0;
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++,i++)
    if ((*it)->desc().auto_refresh())
      (*it)->reset();
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

void Cds::clear_used()
{
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    (*it)->desc().used(false);
  }
}

void Cds::reset_plots()
{
  for (EnList::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    (*it)->reset();
  }
}

void Cds::subscribe(Cdu& user)
{
  _users.push_back(&user);
}

void Cds::unsubscribe(Cdu& user)
{
  _users.remove(&user);
}
