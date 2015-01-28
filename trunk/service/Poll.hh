#ifndef Pds_Poll_hh
#define Pds_Poll_hh

/**
 **  class Poll - serializes access to several stream objects (Ami::Fd)
 **               in a single thread (Ami::Task).  One of the stream
 **               objects is the Ami::Loopback class for queueing 
 **               operations to the polling thread.
 **/

#include "ami/service/Routine.hh"
#include "ami/service/Semaphore.hh"

#include <poll.h>
#include <pthread.h>
#include <string>
using std::string;

class iovec;

namespace Ami {

  class Fd;
  class Task;
  class Socket;

  class Poll : public Routine {
  public:
    Poll(int timeout, const char* s="N/A");
    ~Poll();
  public:
    ///  Start servicing input from the streams
    void start();
    ///  Stop servicing input from the streams
    void stop ();
    ///  The routine which handles input
    void routine();
  public:
    ///  The number of streams managed
    int  nfds() const;
    ///  Accessor to one of the streams
    Fd&  fds(int) const;
    ///  Add a file descriptor to poll list (only safe when poll not running)
    void manage  (Fd&);
  public:
    ///  Add a file descriptor to poll list from another thread (blocks)
    void manage_p(Fd&);
    ///  Stop managing a stream
    virtual void unmanage(Fd&);
    ///  Broadcast a message to all handlers of managed streams
    void bcast_in (const char*,int);
    ///  Broadcast a message out to file descriptors of all managed streams
    void bcast_out(const char*,int);
    void bcast_out(const iovec*,int);
    ///  Post a message to our loopback stream (queue to polling thread)
    void post     (const char*,int);
  private:
    void bcast   (const char*,int,int);
    void bcast   (const iovec*,int,int);
    int poll();
    void adjust ();
  protected:
    int  timeout() const;
    void timeout(int);
  private:
    virtual int processTmo();
    virtual int processIn (const char*,int);
  private:
    int        _timeout;
    string     _context;
    Task*      _task;
    Socket*    _loopback;
    int        _nfds;
    int        _maxfds;
    Fd**       _ofd;
    pollfd*    _pfd;
    Semaphore  _msem;
    char*      _buffer;
    pthread_mutex_t _shutdown_mutex;
    pthread_cond_t  _shutdown_cond;
    bool            _shutdown_compl;
    bool            _shutdown;
  };

};

#endif
