#ifndef XTC_MONITOR_CLIENT_HH
#define XTC_MONITOR_CLIENT_HH

#include "ami/service/Fd.hh"

//#ifdef _POSIX_MESSAGE_PASSING
#include <mqueue.h>
//#endif

namespace Pds { class Dgram; }

namespace Ami {

  class XtcMonitorClient : public Fd {
  public:
    XtcMonitorClient(char* partitionTag);
    ~XtcMonitorClient() {};
  public:
    int fd() const { return _inputQueue; }
    int processIo();
  public:
    virtual void processDgram(Pds::Dgram*);
  private:
    char* _tag;
    mqd_t _inputQueue;
    mqd_t _outputQueue;
    unsigned _sizeOfShm;
    char* _myShm;
  };
}
#endif
