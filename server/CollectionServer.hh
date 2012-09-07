#ifndef Ami_CollectionServer
#define Ami_CollectionServer

#include "ami/server/Server.hh"
#include "ami/client/AbsClient.hh"
#include "ami/data/Message.hh"

namespace Ami {

  class Socket;
  class ConnectionManager;
  class ClientManager;
  class Ins;

  class CollectionServer : public Server,
                           public AbsClient {
  public:
    CollectionServer(unsigned   interface,
                     unsigned   serverGroup,
                     ConnectionManager&,
                     Socket*,
                     bool       postService);
    ~CollectionServer();
  public:
    int processIo();
    int processIo(const char*,int);
  public:
    void connected       ();
    void disconnected    ();
    int  configure       (iovec*);
    int  configured      ();
    void discovered      (const DiscoveryRx&);
    int  read_description(Socket&,int);
    int  read_payload    (Socket&,int);
    bool svc             () const;
    void process         ();
  private:
    void _reply  (Message::Type, Socket&, int);
  private:
    ClientManager* _client_manager;
    bool           _post_service;
  };

};

#endif
