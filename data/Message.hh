#ifndef Ami_MESSAGE_HH
#define Ami_MESSAGE_HH

#include "ami/data/EntryList.hh"

#include <stdint.h>

class iovec;

namespace Ami {

  class Message {
  public:
    enum Type{NoOp,
	      Hello,
	      Connect,
	      Reconnect,
	      Disconnect,
	      DiscoverReq,
	      Discover,
	      ConfigReq,
	      DescriptionReq, 
	      Description, 
              BeginRunReq,
              EndRunReq,
              RefreshReq,
	      PayloadReq, 
	      Payload,
	      PayloadFragment};
    enum { Push=0x40000000 };

    Message(unsigned id, Type type, unsigned payload=0, unsigned offset=0);
    Message(unsigned id, Type type, const EntryList&);
    Message(const Message&);
  public:
    unsigned id     () const;
    Type     type   () const;
    unsigned offset () const;
    unsigned payload() const;
    EntryList list  () const;
    const char* type_str() const;
  public:
    void     id(unsigned);
    void     type(Type t);
    void     payload(const iovec* iov, unsigned iovcnt);
    void     payload(unsigned size);
    void     offset (unsigned size);
    bool     post_service() const;
  private:
    volatile uint32_t  _id;
    uint32_t  _type;
    uint32_t  _offset;
    uint32_t  _payload;
  };
};

#endif
