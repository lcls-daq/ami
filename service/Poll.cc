#include "ami/service/Poll.hh"

#include "ami/service/Fd.hh"
#include "ami/service/Loopback.hh"

#include "ami/service/Task.hh"
#include "ami/service/TaskObject.hh"
#include "ami/service/Sockaddr.hh"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

//#define DBUG

using namespace Ami;

const int Step=32;
const int BufferSize=0x100000;

enum LoopbackMsg { BroadcastIn, BroadcastOut, Shutdown, PostIn, Manage };

Poll::Poll(int timeout, const char* s) :
  _timeout (timeout),
  _context (s),
  _task    (new Task(TaskObject("vmonMgr"))),
  _loopback(new Loopback),
  _nfds    (1),
  _maxfds  (Step),
  _ofd     (new    Fd*[Step]),
  _pfd     (new pollfd[Step]),
  _msem    (Semaphore::EMPTY),
  _buffer  (new char[BufferSize]),
  _shutdown(false)
{
  pthread_mutex_init(&_shutdown_mutex,NULL);
  pthread_cond_init (&_shutdown_cond,NULL);
  _shutdown_compl = false;

  _pfd[0].fd = _loopback->socket();
  _pfd[0].events = POLLIN | POLLERR;
  _pfd[0].revents = 0;
}


Poll::~Poll()
{
#ifdef DBUG
  printf("~Poll %p\n",this);
#endif

  stop();
  // assumes we are stopped
  for (unsigned short n=1; n<_nfds; n++) {
    Fd* fd = _ofd[n];
    if (fd) {
      unmanage(*fd);
      delete fd;
    }
  }
  _task->destroy_b();
  delete[] _ofd;
  delete[] _pfd;
  delete[] _buffer;
  delete _loopback;

  pthread_mutex_destroy(&_shutdown_mutex);
  pthread_cond_destroy (&_shutdown_cond );
}

void Poll::start()
{
  pthread_mutex_lock(&_shutdown_mutex);
  _shutdown=false;
  _shutdown_compl=false;
  pthread_mutex_unlock(&_shutdown_mutex);

  _task->call(this);
}

void Poll::stop()
{
  int msg=Shutdown;
  _loopback->write(&msg,sizeof(int));

  int timeout = _timeout;
  _timeout = 10;

  pthread_mutex_lock(&_shutdown_mutex);
  _shutdown=true;
  while(1) {
    if (_shutdown_compl) break;
    if (pthread_cond_wait(&_shutdown_cond,&_shutdown_mutex))
      perror("Poll::stop cond_wait failed");
  }
  pthread_mutex_unlock(&_shutdown_mutex);

  _timeout = timeout;
}

//
//  Broadcast a message to all remote endpoints
//
void Poll::bcast_out(const char* msg, int size)
{
  bcast(msg, size, BroadcastOut);
}

void Poll::bcast_out(const iovec* iov, int len)
{
  bcast(iov, len, BroadcastOut);
}

void Poll::bcast_in (const char* msg, int size)
{
  bcast(msg, size, BroadcastIn);
}

void Poll::bcast    (const char* msg, int size, int hdr)
{
  int iovcnt=2;
  iovec* iov = new iovec[iovcnt];
  iov[0].iov_base = &hdr      ; iov[0].iov_len = sizeof(hdr);
  iov[1].iov_base = (void*)msg; iov[1].iov_len = size;
  _loopback->writev(iov,iovcnt);
  delete[] iov;
//   _loopback->write(&hdr ,sizeof(hdr));
//   _loopback->write(&bsiz,sizeof(size));
//   _loopback->write(msg  ,size);
}

void Poll::bcast    (const iovec* iov, int len, int hdr)
{
  int iovcnt = len+1;
  iovec* niov = new iovec[iovcnt];
  niov[0].iov_base = &hdr ; niov[0].iov_len = sizeof(hdr);
  for(int i=0; i<len; i++) {
    niov[i+1].iov_base = iov[i].iov_base;
    niov[i+1].iov_len  = iov[i].iov_len;
  }
  _loopback->writev(niov ,iovcnt);
  delete[] niov;
//   _loopback->write(&hdr ,sizeof(hdr));
//   _loopback->write(&size,sizeof(size));
//   _loopback->writev(iov ,len);
}

//
//  Post a message to ourselves in the polling thread
//
void Poll::post    (const char* msg, int size)
{
  int hdr = PostIn;
  int iovcnt=2;
  iovec* iov = new iovec[iovcnt];
  iov[0].iov_base = &hdr      ; iov[0].iov_len = sizeof(hdr);
  iov[1].iov_base = (void*)msg; iov[1].iov_len = size;
  _loopback->writev(iov,iovcnt);
  delete[] iov;
}

void Poll::manage_p(Fd& fd)
{
  int hdr = Manage;
  Fd* p = &fd;
  int iovcnt=2;
  iovec* iov = new iovec[iovcnt];
  iov[0].iov_base = &hdr      ; iov[0].iov_len = sizeof(hdr);
  iov[1].iov_base = &p        ; iov[1].iov_len = sizeof(p);
  _loopback->writev(iov,iovcnt);
  delete[] iov;
  _msem.take();
}

//
//  Should only be called before "start" or within poll thread;
//  i.e. from processIo()
//
void Poll::manage(Fd& fd)
{
#ifdef DBUG
  printf("Poll %p  manage %d\n",this,fd.fd());
#endif

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
#ifdef DBUG
  printf("Poll %p  unmanage %d\n",this,fd.fd());
#endif

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
  socklen_t addrlen = sizeof(sockaddr_in);
  sockaddr_in name;
  if (::getsockname(_pfd[0].fd, (sockaddr*)&name, &addrlen) < 0) {
    perror("  Poll::poll::getsockname");
  }

  int result = 1;
  if (::poll(_pfd, _nfds, _timeout) > 0) {
    if (_pfd[0].revents & (POLLIN | POLLERR)) {
      int size;
      int& cmd = *reinterpret_cast<int*>(_buffer);
      if ((size=_loopback->read(_buffer,BufferSize))<=0)
	printf("Error reading loopback\n");
      else if (cmd==Shutdown)
	result = 0;
      else {
	size -= sizeof(int);
	if (size<0)
	  printf("Error reading bcast\n");
	else {
	  const char* payload = _buffer+sizeof(int);
          if (cmd==BroadcastIn || cmd==BroadcastOut) {
            for (unsigned short n=1; n<_nfds; n++) {
              if (_ofd[n]) {
                if (cmd==BroadcastOut) {
                  ::write(_ofd[n]->fd(), payload, size);
#ifdef DBUG
                  const uint32_t* d = reinterpret_cast<const uint32_t*>(payload);
                  printf("Poll:Bcast_Out skt %d %08x:%08x:%08x:%08x sz %d\n",
                         _ofd[n]->fd(), 
                         d[0],d[1],d[2],d[3],
                         size);
#endif
                }
                else if (!_ofd[n]->processIo(payload,size)) {
                  Fd* fd = _ofd[n];
                  unmanage(*fd);
                  delete fd;
                }
              }
            }
          }
          else if (cmd==PostIn)
            processIn(payload,size);
	  else if (cmd==Manage) {
	    Fd* fd = *reinterpret_cast<Fd**>(const_cast<char*>(payload));
	    manage(*fd);
            _msem.give();
	    return result;  // not safe to loop over _ofd's below
	  }
	}
      }
    }
    for (unsigned short n=1; n<_nfds; n++) {
      if (_ofd[n] && (_pfd[n].revents & (POLLIN | POLLERR)))
	if (!_ofd[n]->processIo()) {
          Fd* fd = _ofd[n];
          unmanage(*fd);
          delete fd;
        }
    }
  }
  else {
    result = processTmo();
    if (_shutdown) result=0;
  }
  return result;
}

int Poll::processTmo() 
{
  return 1; 
}

int Poll::processIn(const char*, int)
{
  return 1;
}

void Poll::routine()
{
  while(poll());

  pthread_mutex_lock(&_shutdown_mutex);
  _shutdown_compl=true;
  pthread_cond_signal(&_shutdown_cond);
  pthread_mutex_unlock(&_shutdown_mutex);
}

int  Poll::nfds() const 
{
  int nfds(0);
  for(unsigned short i=1; i<_nfds; i++) {
    if (_ofd[i])
      nfds++;
  }
  return nfds;
}

Fd&  Poll::fds (int i) const { return *_ofd[i]; }

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
