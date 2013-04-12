//
//  AnalysisServer handles all client requests for data description and payload
//
#include "AnalysisServer.hh"

#include "ami/service/Socket.hh"
#include "ami/server/Factory.hh"
#include "ami/data/Discovery.hh"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

using namespace Ami;

static const int BufferSize = 32*1024;

static const char* _cds_name(Socket& s)
{
  static char buff[32];
  sprintf(buff,"AnalysisServer_%d",s.socket());
  return buff;
}

AnalysisServer::AnalysisServer(Socket*         socket,
                               Factory&        factory) :
  Server(socket),
  _cds   (_cds_name(*socket)),
  _factory(factory)
{
}

AnalysisServer::~AnalysisServer()
{
  _factory.remove(fd());
}

int AnalysisServer::processIo()
{
  int r = 1;
  Message request(0,Message::NoOp);
  int result = _socket->read(&request,sizeof(request));
  if (result != sizeof(request)) {
    printf("S processIo read result %d (skt %d): %s\n",
	   result,_socket->socket(),strerror(errno));
    return 0;
  }

#ifdef DBUG
  printf("AS processIo %d:%d %x\n", request.id(), request.type(), request.payload());
#endif

  switch(request.type()) {
  case Message::Disconnect:
    _described=false;
    r = 0;
    break;
  case Message::DiscoverReq:
    { DiscoveryTx tx(_factory.features(),_factory.discovery());
      int n = tx.niovs()+1;
      _adjust(n);
      tx.serialize(_iov+1);
      reply(request.id(), Message::Discover, n); }
    break;
  case Message::ConfigReq:
    //
    //  Hack: break this read into two pieces to detect the errant
    //        "disconnect" that sometimes interrupts this message.
    //        I tried protecting the sending messages (ClientManager)
    //        with a semaphore, but it did not help.  I don't know how
    //        the ConfigReq message gets preempted.
    //
    if (request.payload() >= sizeof(Message)) {
      timespec tv; clock_gettime(CLOCK_REALTIME,&tv);
      _socket->read(_buffer,sizeof(Message));
      timespec tw; clock_gettime(CLOCK_REALTIME,&tw);
      const Message& msg = *reinterpret_cast<const Message*>(_buffer);
      if (msg.id() == request.id()+1 &&
          msg.type() == Message::Disconnect) {
        printf("%s intercepted disconnected config request\n",_cds_name(*_socket));
	printf("\tconfig req id %d  payload %d\n",request.id(),request.payload());
	printf("\tdiscon req id %d  readtime %fs\n",msg.id(),
	       double(tw.tv_sec-tv.tv_sec)+1.e-9*(double(tw.tv_nsec)-double(tv.tv_nsec)));
        return 0;
      }
      _socket->read(_buffer+sizeof(Message),request.payload()-sizeof(Message));
    }
    else {
      _socket->read(_buffer,request.payload());
    }
    _factory.configure(fd(), request,_buffer,_cds);
    _described = true;
  case Message::DescriptionReq:
    { int n = _cds.description()+1;
      _adjust(n);
      _cds.description(_iov+1);
      reply(request.id(), Message::Description, n); }
    break;
  case Message::PayloadReq:
    if (_described) {
      int n = _cds.payload()+1;
      _adjust(n);
      n = _cds.payload(_iov+1, request.list())+1;
      reply(request.id(), Message::Payload, n); 
      //      _cds.invalidate_payload();
    }
    break;
  default:
    break;
  }

  return r;
}

int AnalysisServer::processIo(const char* msg,int size)
{
  const Message& request = *reinterpret_cast<const Message*>(msg);

#ifdef DBUG
  printf("AS processIo %d:%d %x\n", request.id(), request.type(), request.payload());
#endif

  switch(request.type()) {
  case Message::DiscoverReq:
    _described = false;
    { DiscoveryTx tx(_factory.features(),_factory.discovery());
      int n = tx.niovs()+1;
      _adjust(n);
      tx.serialize(_iov+1);
      reply(request.id(), Message::Discover, n); }
    break;
  default:
    break;
  }
  return 1;
}

