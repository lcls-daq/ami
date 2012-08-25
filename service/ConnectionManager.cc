#include "ConnectionManager.hh"

#include "Port.hh"
#include "Sockaddr.hh"
#include "Task.hh"
#include "TSocket.hh"
#include "ConnectionHandler.hh"

using namespace Ami;

ConnectionManager::ConnectionManager(int interface) :
  _socket    (new TSocket),
  _task      (new Task(TaskObject("lstn"))),
  _listen_sem(Semaphore::EMPTY),
  _connect_id(0)
{
  unsigned short port = Port::clientPortBase();
  unsigned short _port(0);
  do {
    _port=port;
    Ins ins(interface,_port);
    //    try { _socket->bind(ins); }
    //    catch(Event& e) {
    //      printf("bind error : %s : trying port %d\n",
    //             e.what(),_port);
    //      port++;
    //    }
    if (!_socket->bind(ins)) {
      perror("ConnectionManager bind error (retrying)");
      port++;
    }
  } while(_port!=port);

  _task->call(this);
  _listen_sem.take();
}

ConnectionManager::~ConnectionManager()
{
}

Ins ConnectionManager::ins() const
{
  return _socket->ins();
}

unsigned ConnectionManager::add(ConnectionHandler& h)
{
  _handlers.push_back(&h);
  return _connect_id++;
}

void ConnectionManager::remove(ConnectionHandler& h)
{
  _handlers.remove(&h);
}


void ConnectionManager::routine()
{
  printf("ConnectionManager listening on %x/%d\n", 
         _socket->ins().address(), 
         _socket->ins().portId());
  
  while(1) {
    if (::listen(_socket->socket(),5)<0)
      printf("ConnectionManager listen failed\n");
    else {
      _listen_sem.give();
      Ami::Sockaddr name;
      unsigned length = name.sizeofName();
      int s = ::accept(_socket->socket(),name.name(), &length);
      if (s<0)
	printf("ConnectionManager accept failed\n");
      else {
        uint32_t connect_id;
        if (::read(s,&connect_id,sizeof(connect_id))<0) {
          printf("ConnectionManager read %d failed\n",s);
          ::close(s);
        }
        else {
          for(std::list<ConnectionHandler*>::iterator it = _handlers.begin();
              it!=_handlers.end(); it++)
            if ((*it)->connection_id() == connect_id) {
              (*it)->handle(s);
              break;
            }
        }
      }
    }
    
  }
}

unsigned ConnectionManager::receive_bytes() 
{
  unsigned rxb=0;
  for(std::list<ConnectionHandler*>::iterator it = _handlers.begin();
      it!=_handlers.end(); it++)
    rxb += (*it)->receive_bytes();
  return rxb;
}
