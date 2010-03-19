#include <string.h>
#include <sys/uio.h>
#include <math.h>

#include "ami/data/Entry.hh"
#include "pdsdata/xtc/ClockTime.hh"

using namespace Ami;

Entry::Entry() : 
  _payloadsize(0),
  _payload(0)
{}

Entry::~Entry()
{
  delete [] _payload;  
}

void Entry::payload(iovec& iov)
{
  iov.iov_base = _payload;
  iov.iov_len = _payloadsize;
}

void Entry::payload(iovec& iov) const
{
  iov.iov_base = static_cast<void*>(_payload);
  iov.iov_len = _payloadsize;
}

void Entry::reset()
{
  memset(_payload, 0, _payloadsize);
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

static const Pds::ClockTime _invalid(0,0);

void Entry::valid(const Pds::ClockTime& t) 
{
  *_payload = *(reinterpret_cast<const unsigned long long*>(&t));
}

void Entry::valid(double v)
{
  Pds::ClockTime t(unsigned(v),unsigned(drem(v,1)*1.e9));
  *_payload = *reinterpret_cast<unsigned long long*>(&t);
}

void Entry::invalid() { valid(_invalid); }

bool Entry::valid() const { return !(time()==_invalid); }
