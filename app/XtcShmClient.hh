#ifndef Ami_XtcShmClient_hh
#define Ami_XtcShmClient_hh

#include "ami/service/EventFd.hh"
#include "pdsdata/app/XtcMonitorClient.hh"

#include "ami/app/XtcClient.hh"
#include "ami/service/Semaphore.hh"

namespace Ami {

  class Task;

  class XtcShmClient : public XtcMonitorClient,
		       public EventFd {
  public:
    XtcShmClient(XtcClient& client, char* partitionTag, int index=0);
    ~XtcShmClient() {};
  public:
    int processDgram(Dgram*);
    int fd() const;
    int processIo();
  private:
    XtcClient& _client;
    int        _pipefd[2];
    Semaphore  _sem;
    Task*      _task;
  };
};
#endif
