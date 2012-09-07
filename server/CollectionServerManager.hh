#ifndef Ami_CollectionServerManager_hh
#define Ami_CollectionServerManager_hh

#include "ami/server/ServerManager.hh"

namespace Ami {

  class ConnectionManager;
  class Socket;

  class CollectionServerManager : public ServerManager {
  public:
    CollectionServerManager (unsigned client_interface,
                             unsigned client_group,
                             unsigned server_interface,
                             unsigned server_group);
    ~CollectionServerManager();
  public:
    Server* new_server(Socket*, bool);
  private:
    ConnectionManager* _connection_manager;
    unsigned           _server_interface;
    unsigned           _server_group;
  };
};

#endif
