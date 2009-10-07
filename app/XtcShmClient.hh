#ifndef Ami_XtcShmClient_hh
#define Ami_XtcShmClient_hh

#include "ami/service/Fd.hh"

#include "ami/app/XtcClient.hh"

//#ifdef _POSIX_MESSAGE_PASSING
#include <mqueue.h>
//#endif

namespace Pds { class Dgram; }

namespace Ami {

  class XtcShmClient : public Fd {
  public:
    XtcShmClient(XtcClient& client, char* partitionTag);
    ~XtcShmClient() {};
  public:
    int fd() const { return _inputQueue; }
    int processIo();
  private:
    XtcClient& _client;
    char* _tag;
    mqd_t _inputQueue;
    mqd_t _outputQueue;
    unsigned _sizeOfShm;
    char* _myShm;
  };
}
#endif
