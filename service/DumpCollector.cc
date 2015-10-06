#include "ami/service/DumpCollector.hh"
#include "ami/service/DumpSource.hh"
#include "ami/service/Task.hh"
#include "ami/service/TSocket.hh"

#include <poll.h>
#include <unistd.h>

#include <sstream>

using namespace Ami;

static Ins _ins(0xefff18ff, 5732);

Ins DumpCollector::ins() { return _ins; }

DumpCollector::DumpCollector(int interface,
                             int secondary) :
  _task( new Task(TaskObject("amidmp"))),
  _srv ( _ins, interface ),
  _sec (secondary)
{
  if (_sec == 0x7f000001) _sec=0;
  _task->call(this);
}

DumpCollector::~DumpCollector()
{
  _task->destroy();
}

void DumpCollector::add(const DumpSource& s)
{
  _servers.push_back(&s);
}

void DumpCollector::routine()
{
  pollfd pfd[1];
  pfd[0].fd = _srv.socket();
  pfd[0].events = POLLIN | POLLERR;
  pfd[0].revents = 0;

  unsigned nfd=1;

  int tmo = 2000;

  if (::poll(pfd,nfd,tmo) > 0) {
    Ins remote;
    while (_srv.read(&remote,sizeof(remote))>0) {
      if (remote.address()==_sec) continue;
      printf("DumpCollector connecting to %x/%d\n",
	     remote.address(),remote.portId());
      std::ostringstream o;
      
      char buff[128];
      gethostname(buff,128);

      o << buff << ":" << getpid() << std::endl;

      for(std::list<const DumpSource*>::iterator it=_servers.begin();
	  it!=_servers.end(); it++) {
	o << (*it)->dump();
      }

      try {
        TSocket s;
        s.connect(remote);
        s.write(o.str().c_str(),o.str().size());

        if (_sec) {
          unsigned short port = 32768;
          TSocket _listen;
          while(!_listen.bind(Ins(_sec,port)))
            port++;

          Ins ins(_listen.ins());

          if (::listen(_listen.socket(),5)<0)
            printf("Listen failed\n");
          else {
            int so = ::socket(AF_INET, SOCK_DGRAM, 0);
            in_addr address;
            address.s_addr = htonl(_sec);
            if (setsockopt(so, IPPROTO_IP, IP_MULTICAST_IF,
                           (char*)&address, sizeof(in_addr))<0) {
              perror("Error setting up multicast transmit interface");
              break;
            }
      
            Ins dins(DumpCollector::ins());
            Sockaddr dst(dins);
            unsigned dlen = dst.sizeofName();
            if (::connect(so, dst.name(), dlen)<0) {
              perror("Binding to destination");
              break;
            }
      
            if (::send(so, &ins, sizeof(ins),0)<0) {
              perror("Sending local info");
              break;
            }
          }

          pollfd pfd[1];
          pfd[0].fd = _listen.socket();
          pfd[0].events = POLLIN | POLLERR;
          pfd[0].revents = 0;
          unsigned nfd = 1;
          int tmo = 1000;

          while(poll(pfd,nfd,tmo)>0) {
            Sockaddr name;
            unsigned length = name.sizeofName();
            int so = ::accept(_listen.socket(),name.name(), &length);
            if (so<0)
              printf("Accept failed\n");
            else {
              TSocket _accepted(so);
              const int sz = 0x8000;
              char buff[sz];
              int  nb;
              while((nb=_accepted.read(buff,sz))>0) {
                buff[nb]=0;
                s.write(buff,nb);
              }
            }
          }
        }
        break;
      }
      catch(Event& e) {
        printf("connect to %x.%d failed\n",remote.address(),remote.portId());
        printf("caught exception %s:%s\n",e.who(),e.what());
      }
    }
  }

  _task->call(this);
}
