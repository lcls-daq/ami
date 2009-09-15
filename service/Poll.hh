#ifndef Pds_Poll_hh
#define Pds_Poll_hh

#include "ami/service/Routine.hh"
#include "ami/service/Semaphore.hh"

#include <poll.h>

namespace Ami {

  class Fd;
  class Task;
  class Socket;

  class Poll : public Routine {
  public:
    Poll(int timeout);
    ~Poll();
  public:
    void start();
    void stop ();
    void routine();
  public:
    void manage  (Fd&);
    void unmanage(Fd&);
    void bcast   (const char*,int);
  private:
    int poll();
    void adjust ();
  protected:
    int  timeout() const;
    void timeout(int);
  private:
    virtual int processTmo() = 0;
  private:
    int        _timeout;
    Task*      _task;
    Socket*    _loopback;
    int        _nfds;
    int        _maxfds;
    Fd**       _ofd;
    pollfd*    _pfd;
    Semaphore  _sem;
    char*      _buffer;
  };

};

#endif
