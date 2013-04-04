#include "ConnectionManager.hh"

#include "Port.hh"
#include "Sockaddr.hh"
#include "Task.hh"
#include "TSocket.hh"
#include "ConnectionHandler.hh"

//#define DBUG

using namespace Ami;

ConnectionManager::ConnectionManager(int interface) :
  _socket    (new TSocket),
  _task      (new Task(TaskObject("lstn"))),
  _listen_sem(Semaphore::EMPTY),
  _list_sem  (Semaphore::FULL),
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
  _list_sem.take();
  _handlers.push_back(&h);
  _list_sem.give();
#ifdef DBUG
  printf("ConnMgr added %d\n",_connect_id);
#endif
  return _connect_id++;
}

void ConnectionManager::remove(ConnectionHandler& h)
{
  _list_sem.take();
  _handlers.remove(&h);
  _list_sem.give();
}


void ConnectionManager::routine()
{
  printf("ConnectionManager listening on %x/%d\n", 
         _socket->ins().address(), 
         _socket->ins().portId());
  
  while(1) {
    if (::listen(_socket->socket(),10)<0)
      printf("ConnectionManager listen failed\n");
    else {
      _listen_sem.give();
      Ami::Sockaddr name;
      unsigned length = name.sizeofName();
      int s = ::accept(_socket->socket(),name.name(), &length);
      if (s<0) {
	perror("ConnectionManager accept failed");
        abort();
      }
      else {
        uint32_t connect_id;
        if (::read(s,&connect_id,sizeof(connect_id))<0) {
          printf("ConnectionManager read %d failed\n",s);
          ::close(s);
        }
        else {
#ifdef DBUG
          printf("ConnMgr handling skt %d  connid %d\n",s,connect_id);
#endif
          _list_sem.take();
          for(std::list<ConnectionHandler*>::iterator it = _handlers.begin();
              it!=_handlers.end(); it++)
            if ((*it)->connection_id() == connect_id) {
              (*it)->handle(s);
              break;
            }
          _list_sem.give();
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
