#include "ami/service/Poll.hh"

#include "ami/service/Fd.hh"
#include "ami/service/Loopback.hh"

#include "ami/service/Task.hh"
#include "ami/service/TaskObject.hh"

#include <errno.h>
#include <string.h>

using namespace Ami;

const int Step=32;
const int BufferSize=0x1000;

enum LoopbackMsg { Broadcast, Shutdown };

Poll::Poll(int timeout) :
  _timeout (timeout),
  _task    (new Task(TaskObject("vmonMgr"))),
  _loopback(new Loopback),
  _nfds    (1),
  _maxfds  (Step),
  _ofd     (new    Fd*[Step]),
  _pfd     (new pollfd[Step]),
  _sem     (Semaphore::EMPTY),
  _buffer  (new char[BufferSize])
{
  _pfd[0].fd = _loopback->socket();
  _pfd[0].events = POLLIN | POLLERR;
  _pfd[0].revents = 0;
}


Poll::~Poll()
{
  _task->destroy();
  delete[] _ofd;
  delete[] _pfd;
  delete[] _buffer;
}

void Poll::start()
{
  _task->call(this);
}

void Poll::stop()
{
  int msg=Shutdown;
  _loopback->write(&msg,sizeof(msg));
  _sem.take();
}

//
//  Broadcast a message to all servers
//
void Poll::bcast(const char* msg, int size)
{
  int hdr=Broadcast;
  _loopback->write(&hdr,sizeof(hdr));
  _loopback->write(msg,size);
}

//
//  Should only be called before "start" or within poll thread;
//  i.e. from processIo()
//
void Poll::manage(Fd& fd)
{
  unsigned available = 0;
  for (unsigned short n=1; n<_nfds; n++) {
    if (!_ofd[n] && !available) available = n;
    if (_ofd[n] == &fd) return;
  }
  if (!available) {
    if (_nfds == _maxfds) adjust();
    available = _nfds++;
  }
  _ofd[available] = &fd;
  _pfd[available].fd = fd.fd();
  _pfd[available].events = POLLIN | POLLERR;
  _pfd[available].revents = 0;
}

void Poll::unmanage(Fd& fd)
{
  for (unsigned short n=1; n<_nfds; n++) {
    if (_ofd[n] == &fd) {
      _ofd[n] = 0;
      _pfd[n].fd = -1;
      _pfd[n].events = 0;
      _pfd[n].revents = 0;
    }
  }
}

int Poll::poll()
{
  int result = 1;
  if (::poll(_pfd, _nfds, _timeout) > 0) {
    if (_pfd[0].revents & (POLLIN | POLLERR)) {
      int cmd;
      _loopback->read(&cmd,sizeof(cmd));
      if (cmd==Shutdown)
	result = 0;
      else if (cmd==Broadcast) {
	int size=_loopback->read(_buffer,BufferSize);
	for (unsigned short n=1; n<_nfds; n++) {
	  if (_ofd[n])
	    if (!_ofd[n]->processIo(_buffer,size))
	      unmanage(*_ofd[n]);
	}
      }
    }
    for (unsigned short n=1; n<_nfds; n++) {
      if (_ofd[n] && (_pfd[n].revents & (POLLIN | POLLERR)))
	if (!_ofd[n]->processIo())
	  unmanage(*_ofd[n]);
    }
  }
  else {
    result = processTmo();
  }
  return result;
}

void Poll::routine()
{
  while(poll());
  _sem.give();
}

int  Poll::timeout() const { return _timeout; }

void Poll::timeout(int tmo) { _timeout=tmo; }

void Poll::adjust()
{
  unsigned short maxfds = _maxfds + Step;

  Fd** ofd = new Fd*[maxfds];
  memcpy(ofd, _ofd, _nfds*sizeof(Fd*));
  delete [] _ofd;
  _ofd = ofd;

  pollfd* pfd = new pollfd[maxfds];
  memcpy(pfd, _pfd, _nfds*sizeof(pollfd));
  delete [] _pfd;
  _pfd = pfd;

  _maxfds = maxfds;
}
