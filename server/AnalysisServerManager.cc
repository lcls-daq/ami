#include "AnalysisServerManager.hh"

#include "ami/server/AnalysisServer.hh"

using namespace Ami;


AnalysisServerManager::AnalysisServerManager(unsigned interface,
                                             unsigned serverGroup) :
  ServerManager(interface, serverGroup),
  _factory(0)
{
}

AnalysisServerManager::~AnalysisServerManager()
{
}

Server* AnalysisServerManager::new_server(Socket* s, bool)
{
  return new AnalysisServer(s,*_factory);
}

void AnalysisServerManager::serve(Factory& factory, Semaphore* sem)
{
  _factory = &factory;
  ServerManager::serve(sem);
}

