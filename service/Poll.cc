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

enum LoopbackMsg { BroadcastIn, BroadcastOut, Shutdown, PostIn, Manage, None };

class LMsg {
public:
  LMsg() : _hdr(None), _size(0), _payload(0) {}
  LMsg(int hdr, const char* p, int size) : 
    _hdr(hdr), _size(size), _payload(new char[size]) 
  { memcpy(_payload, p, size); }
  LMsg(int hdr, const iovec* iov, int len) :
    _hdr(hdr), _size(0)
  {
    for(int i=0; i<len; i++)
      _size += iov[i].iov_len;
    char* p = _payload = new char[_size];
    for(int i=0; i<len; i++) {
      memcpy(p, iov[i].iov_base, iov[i].iov_len);
      p += iov[i].iov_len;
    }
  }
  LMsg(int hdr) : _hdr(hdr), _size(0), _payload(0) {}
  LMsg(Fd& p) :
    _hdr(Manage), _size(0), _payload(reinterpret_cast<char*>(&p)) {}
public:
  LoopbackMsg cmd    () const { return (LoopbackMsg)_hdr; }
  int         size   () const { return _size; }
  char*       payload() const { return _payload; }
public:
  void        free   ();
private:
  int _hdr;
  int _size;
  char* _payload;
};

void LMsg::free()
{
  try {
    if (_size) delete[] _payload;
  }
  catch (const std::exception& ex) {
    printf("LMsg::free caught exception:\n");
    printf("\tthis: %p\n",this);
    printf("\t_hdr: %x\n",_hdr);
    printf("\t_size: %x\n",_size);
    printf("\t_payload: %p\n",_payload);
  }
}

const int Step=32;
const int BufferSize=sizeof(LMsg);

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
  LMsg lmsg(Shutdown);
  _loopback->write(&lmsg,sizeof(lmsg));

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
  LMsg lmsg(hdr, msg, size);
  _loopback->write(&lmsg, sizeof(lmsg));
}

void Poll::bcast    (const iovec* iov, int len, int hdr)
{
  LMsg lmsg(hdr, iov, len);
  _loopback->write(&lmsg, sizeof(lmsg));
}

//
//  Post a message to ourselves in the polling thread
//
void Poll::post    (const char* msg, int size)
{
  LMsg lmsg(PostIn, msg, size);
  _loopback->write(&lmsg, sizeof(lmsg));
}

void Poll::manage_p(Fd& fd)
{
  LMsg lmsg(fd);
  _loopback->write(&lmsg, sizeof(lmsg));
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
      LMsg lmsg;
      int sz;
      if ((sz=_loopback->read(&lmsg,sizeof(lmsg)))<int(sizeof(LMsg))) {
        printf("Error reading loopback:");
        uint32_t* p = reinterpret_cast<uint32_t*>(&lmsg);
        while(sz>0) { printf(" %08x",*p++); sz-=sizeof(uint32_t); }
        printf(". Ignored.\n");
        return result;
      }
      else if (lmsg.cmd()==Shutdown)
        result = 0;
      else {
        if (lmsg.cmd()==BroadcastIn || lmsg.cmd()==BroadcastOut) {
          for (unsigned short n=1; n<_nfds; n++) {
            if (_ofd[n]) {
              if (lmsg.cmd()==BroadcastOut) {
                if (::write(_ofd[n]->fd(), lmsg.payload(), lmsg.size())==-1)
                  perror("Ami::Poll::poll BroadcastOut");
              }
              else if (!_ofd[n]->processIo(lmsg.payload(),lmsg.size())) {
                Fd* fd = _ofd[n];
                unmanage(*fd);
                delete fd;
              }
            }
          }
        }
        else if (lmsg.cmd()==PostIn)
          processIn(lmsg.payload(),lmsg.size());
        else if (lmsg.cmd()==Manage) {
          Fd* fd = reinterpret_cast<Fd*>(const_cast<char*>(lmsg.payload()));
          manage(*fd);
          _msem.give();
          lmsg.free();
          return result;  // not safe to loop over _ofd's below
        }
      }
      lmsg.free();
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
  // pthread_sigmask // block SIGPIPE
  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sigs, NULL);

  while(poll());

  // pthread_sigmask // unblock SIGPIPE

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
