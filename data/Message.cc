#include <sys/uio.h>

#include "Message.hh"

using namespace Ami;

Message::Message(unsigned id, Type type, unsigned payload, unsigned offset) : 
  _id     (id),
  _type   (type),
  _offset (offset),
  _payload(payload)
{}

Message::Message(unsigned id, Type type, const EntryList& l) :
  _id     (id),
  _type   (type)
{
  l.serialize(_offset,_payload);
}

Message::Message(const Message& o) :
  _id     (o._id),
  _type   (o._type),
  _offset (o._offset),
  _payload(o._payload)
{}

void Message::id(unsigned i) { _id = i; }

unsigned Message::id          () const { return _id & ~(1<<31); }

bool     Message::post_service() const { return _id>>31; }

Message::Type Message::type() const {return (Type)_type;}

void Message::type(Type t) {_type = t;}

unsigned Message::offset() const {return _offset;}

unsigned Message::payload() const {return _payload;}

EntryList Message::list() const { return EntryList(_offset, _payload); }

void Message::payload(const iovec* iov, unsigned iovcnt) 
{
  unsigned size = 0;
  for (unsigned n=0; n<iovcnt; n++, iov++) {
    size += (*iov).iov_len;
  }
  _payload = size;
}

void Message::payload(unsigned size) { _payload = size; }

void Message::offset (unsigned size) { _offset  = size; }
