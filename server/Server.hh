#ifndef Ami_Server
#define Ami_Server

#include "ami/service/Fd.hh"
#include "ami/service/DumpSource.hh"

#include "ami/data/Message.hh"

#include <list>
#include <string>

class iovec;

namespace Ami {

  class Socket;

  class Server : public Fd,
		 public DumpSource {
  public:
    Server(Socket*);
    ~Server();
  public:  // Fd interface
    int fd() const;
  protected:
    void _adjust     (int);
    void reply       (unsigned,Message::Type,unsigned);
  public:
    std::string dump() const;
  protected:
    Socket*         _socket;
    iovec*          _iov;
    int             _iovcnt;
    Message         _reply;
    std::list<Message> _history;
    char*           _buffer;
    bool            _described;
  };

};

#endif
