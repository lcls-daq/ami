#include <string.h>
#include <sys/uio.h>
#include <math.h>
#include <new>

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <stdio.h>

#define DBUG

static const uint64_t INVALID_BIT  = 1ULL;
static const uint64_t VALID_MASK = ~INVALID_BIT;

using namespace Ami;

Entry::Entry() : 
  _payloadsize(0),
  _payload(0)
{}

Entry::~Entry()
{
  if (_payload)
    delete [] _payload;  
}

void Entry::payload(iovec& iov) const
{
  iov.iov_base = static_cast<void*>(_payload);
  iov.iov_len = _payloadsize;
}

void Entry::reset()
{
  memset(_payload, 0, _payloadsize);
  invalid();
}

void* Entry::allocate(unsigned size)
{
  if (_payload)
    delete [] _payload;  

  _payloadsize = sizeof(unsigned long long)+size;
  _payload = new unsigned long long[(_payloadsize>>3)+1];

  reset();

  return (_payload+1);
}

double Entry::last() const 
{
  const Pds::ClockTime& t = time();
  return t.seconds() + 1.e-9 * t.nanoseconds();
}

const Pds::ClockTime& Entry::time() const 
{
  return *reinterpret_cast<const Pds::ClockTime*>(_payload);
}

void Entry::valid(const Pds::ClockTime& t) 
{
  *_payload = *(reinterpret_cast<const unsigned long long*>(&t)) & VALID_MASK;
}

void Entry::invalid() 
{ 
  *_payload |= INVALID_BIT;
}

bool Entry::valid() const { return _payload!=0 && ((*_payload)&INVALID_BIT)==0; }

void Entry::merge(char* p) const
{
  unsigned long long* u = reinterpret_cast<unsigned long long*>(p);

  char db_buff[128];
  char* pdb = db_buff;
#ifdef DBUG
  const bool ldbug = desc().type() == DescEntry::Image;
#else
  const bool ldbug = false;
#endif

  if (ldbug)
    pdb += sprintf(pdb,"Entry[%s]::merge %016llx + %016llx",
		   DescEntry::type_str(desc().type()),
		   *_payload, *u);

  bool lvalid = valid();
  bool pvalid = !(*u & INVALID_BIT);

  if (desc().aggregate()) {  // merge the valid data, keeping the latest timestamp
    if (pvalid) {
      if (lvalid) {
	if (*_payload > *u) *u = *_payload;
	_merge((char*)(u+1));
	if (ldbug)
	  pdb += sprintf(pdb," merged   : ts %016llx",*u);
      }
    }
    else if (lvalid) {
      memcpy(p,_payload,_payloadsize);
      if (ldbug)
	pdb += sprintf(pdb," replaced : ts %016llx",*u);
    }
  }

  else { // keep the latest
    if (*_payload > *u) {
      memcpy(p,_payload,_payloadsize);
      if (ldbug)
	pdb += sprintf(pdb," replaced : ts %016llx",*u);
    }
  }

  if (ldbug)
    printf("%s\n",db_buff);
}
