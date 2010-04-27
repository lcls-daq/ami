#ifndef Ami_MESSAGE_HH
#define Ami_MESSAGE_HH

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
	      PayloadReq, 
	      Payload,
	      PayloadFragment};

    Message(unsigned id, Type type, unsigned payload=0, unsigned offset=0);
  public:
    unsigned id     () const;
    Type     type   () const;
    unsigned offset () const;
    unsigned payload() const;
  public:
    void     id(unsigned);
    void     type(Type t);
    void     payload(const iovec* iov, unsigned iovcnt);
    void     payload(unsigned size);
    void     offset (unsigned size);
    
  private:
    volatile unsigned  _id;
    Type      _type;
    unsigned  _offset;
    unsigned  _payload;
  };
};

#endif
