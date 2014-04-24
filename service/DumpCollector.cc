#include "ami/service/DumpCollector.hh"
#include "ami/service/DumpSource.hh"
#include "ami/service/Task.hh"
#include "ami/service/TSocket.hh"

#include <poll.h>

#include <sstream>

using namespace Ami;

static Ins _ins(0xefff18ff, 5732);

Ins DumpCollector::ins() { return _ins; }

DumpCollector::DumpCollector(int interface) :
  _task( new Task(TaskObject("amidmp"))),
  _srv ( _ins, interface )
{
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
    if (_srv.read(&remote,sizeof(remote))>0) {
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

      TSocket s;
      s.connect(remote);
      s.write(o.str().c_str(),o.str().size());
    }
  }

  _task->call(this);
}
