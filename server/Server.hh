#ifndef Ami_Server
#define Ami_Server

#include "ami/service/Fd.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Message.hh"

class iovec;

namespace Ami {

  class VServerSocket;
  class VServerManager;
  class Factory;

  class Server : public Fd {
  public:
    Server(VServerSocket*,
	   Factory&,
	   const Message&);
    ~Server();
  public:  // Fd interface
    int fd() const;
    int processIo();
    int processIo(const char*,int);
  private:
    void _adjust     (int);
    void reply       (unsigned,Message::Type,unsigned);
  private:
    VServerSocket*  _socket;
    iovec*          _iov;
    int             _iovcnt;
    Message         _reply;
    Cds             _cds;
    Factory&        _factory;
    char*           _buffer;
    bool            _described;
  };

};

#endif
