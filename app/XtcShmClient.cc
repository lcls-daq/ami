#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"

#include "ami/app/XtcShmClient.hh"
#include "ami/app/XtcClient.hh"

namespace Pds {
  class Msg {
    public:
      Msg() {}; 
      ~Msg() {}; 
      int bufferIndex() {return _bufferIndex;}
      int numberOfBuffers() {return _numberOfBuffers;}
      int sizeOfBuffers() {return _sizeOfBuffers;}
    private:
      int _bufferIndex;
      int _numberOfBuffers;
      unsigned _sizeOfBuffers;
  };
}

using namespace Ami;

enum {PERMS  = S_IRUSR|S_IRUSR|S_IRUSR|S_IROTH|S_IROTH|S_IROTH|S_IRGRP|S_IRGRP|S_IRGRP};
enum {OFLAGS = O_RDONLY};

XtcShmClient::XtcShmClient(XtcClient& client, char * tag) :
  _client(client),
  _tag(tag)
{
  int error = 0;
  char toServerQname[128] = "/PdsFromMonitorMsgQueue_";
  char fromServerQname[128] = "/PdsToMonitorMsgQueue_"; 
  strcat(toServerQname, tag);
  strcat(fromServerQname, tag);
  struct mq_attr mymq_attr;

  _outputQueue = mq_open(toServerQname, O_WRONLY, PERMS, &mymq_attr);
  if (_outputQueue == (mqd_t)-1) {
    perror("mq_open output");
    error++;
  }
  _inputQueue = mq_open(fromServerQname, O_RDONLY, PERMS, &mymq_attr);
  if (_inputQueue == (mqd_t)-1) {
    perror("mq_open input");
    error++;
  }

  if ((_inputQueue == (mqd_t)-1) || (_outputQueue == (mqd_t)-1)) {
    fprintf(stderr, "Could not open at least one message queue!\n");
    fprintf(stderr, "To Server:%d, From Server:%d\n", _outputQueue, _inputQueue);
    fprintf(stderr, "To Server:%s, From Server:%s\n", toServerQname, fromServerQname);
    error++;
  }

  _sizeOfShm = 0;
}

int XtcShmClient::processIo()
{
  int error = 0;
  unsigned pageSize = (unsigned)sysconf(_SC_PAGESIZE);
  unsigned priority = 0;
  Pds::Dgram* dg = NULL;
  Msg myMsg;
  if (mq_receive(_inputQueue, (char*)&myMsg, sizeof(myMsg), &priority) < 0) {
      perror("mq_receive buffer");
      error++;
  } else {
    if (!_sizeOfShm) {
      _sizeOfShm = myMsg.numberOfBuffers() * myMsg.sizeOfBuffers();
      unsigned remainder = _sizeOfShm % pageSize;
      if (remainder) {
	printf("_sizeOfShm changed from 0x%x", _sizeOfShm);
	_sizeOfShm += pageSize - remainder;
	printf(" to 0x%x\n", _sizeOfShm);
      }
      char shmName[128] = "/PdsMonitorSharedMemory_";
      strcat(shmName, _tag);
      int shm = shm_open(shmName, OFLAGS, PERMS);
      if (shm < 0) perror("shm_open");
      _myShm = (char*)mmap(NULL, _sizeOfShm, PROT_READ, MAP_SHARED, shm, 0);
      if (_myShm == MAP_FAILED) perror("mmap");
      else printf("Shared memory at %x\n", (unsigned)_myShm);
    }
    dg = (Pds::Dgram*) (_myShm + (myMsg.sizeOfBuffers() * myMsg.bufferIndex()));
    _client.processDgram(dg);
    if (mq_send(_outputQueue, (const char *)&myMsg, sizeof(myMsg), priority)) {
      perror("mq_send back buffer");
      error++;
    }
  }
  //  return error ? 0 : 1;
  return 1;
}
