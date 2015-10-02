#ifndef Ami_ServerManager_hh
#define Ami_ServerManager_hh

#include "ami/service/Fd.hh"
#include "ami/service/Poll.hh"
#include "ami/service/DumpSource.hh"

#include <list>
#include <vector>

namespace Ami {

  class Ins;
  class Server;
  class Semaphore;
  class Socket;
  class Message;
  class Routine;
  class EventFd;

  class ServerManager : public Poll,
			public Fd,
			public DumpSource {
  public:
    ServerManager (unsigned interface,
		   unsigned serverGroup);
    virtual ~ServerManager();
  public:
    virtual Server* new_server(Socket*, const Message&) = 0;
  public:
    void serve     (Semaphore* =0);
    void dont_serve();
  public:
    void discover  ();
    void discover_post();
    void beginRun  (unsigned);
    void endRun    (unsigned);
  public:
    void register_key(unsigned, int);
    void discover_key(unsigned);
  public:
    void manage    (EventFd&);
    void manage    (Fd&);
    void unmanage  (Fd&);
  private:   // Poll interface
    virtual int processTmo();
    virtual int processIn (const char*, int);
  public:    // Fd interface
    virtual int fd() const;
    virtual int processIo();
  public:
    std::string dump() const;
  private:
    typedef std::list<Server*> SvList;
    unsigned           _interface;
    unsigned           _serverGroup;
    unsigned           _ppinterface;
    Socket*            _socket;
    SvList             _servers;
    SvList             _uservers;
    Semaphore*         _connect_sem;
    typedef std::list<Fd*> FdList;
    typedef std::vector< FdList > KeyList;
    KeyList            _key_servers;
  protected:
    EventFd*           _event;
  };
};

#endif
