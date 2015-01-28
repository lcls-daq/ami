#include "CollectionServerManager.hh"

#include "ami/server/CollectionServer.hh"
#include "ami/service/ConnectionManager.hh"
#include "ami/data/Message.hh"

using namespace Ami;

CollectionServerManager::CollectionServerManager (unsigned client_interface,
                                                  unsigned client_group,
                                                  unsigned server_interface,
                                                  unsigned server_group) :
  ServerManager      (client_interface,
                      client_group),
  _connection_manager(new ConnectionManager(server_interface)),
  _server_interface  (server_interface),
  _server_group      (server_group)
{
}

CollectionServerManager::~CollectionServerManager()
{
  delete _connection_manager;
}

Server* CollectionServerManager::new_server(Socket* s, const Message& r)
{
  return new CollectionServer(_server_interface,
                              _server_group,
                              *_connection_manager,
                              s,
                              r.post_service());
}
