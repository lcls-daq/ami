#include "ami/service/TSocket.hh"
#include "ami/service/Sockaddr.hh"
#include "ami/service/Ins.hh"
#include "ami/service/DumpCollector.hh"
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <poll.h>

using namespace Ami;

static void usage(const char* p)
{
  printf("Usage: %s [-i <interface_name/dotted_ip>]\n",p);
}

int main(int argc, char* argv[])
{
  unsigned ip_dst = DumpCollector::ins().address();
  unsigned ip_host = 0x7f000001;

  TSocket _listen;

  char c;
  while ((c = getopt(argc, argv, "i:h")) != -1) {
    switch(c) {
    case 'i': 
      { if (optarg[0]<'0' || optarg[0]>'9') {
	  struct ifreq ifr;
	  strcpy( ifr.ifr_name, optarg );
	  int iError = ioctl( _listen.socket(), SIOCGIFADDR, (char*)&ifr );
	  if ( iError == 0 )
	    ip_host = ntohl( *(unsigned int*) &(ifr.ifr_addr.sa_data[2]) );
	  else
	    printf( "Cannot get IP address from network interface %s\n", optarg );
	}
	else {
	  ip_host = ntohl(inet_addr(optarg));
	}
      } break;
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }

  if (optind!=argc) {
    usage(argv[0]);
    exit(-1);
  }

  unsigned short port = 32768;
  while(!_listen.bind(Ins(ip_host,port)))
    port++;

  Ins ins(_listen.ins());

  if (::listen(_listen.socket(),5)<0)
    printf("Listen failed\n");
  else {
      
    printf("Listening on socket %d : %x/%d\n",
	   _listen.socket(),
	   _listen.ins().address(), _listen.ins().portId());
      
    {
      int so = ::socket(AF_INET, SOCK_DGRAM, 0);
      in_addr address;
      address.s_addr = htonl(ip_host);
      if (setsockopt(so, IPPROTO_IP, IP_MULTICAST_IF,
		     (char*)&address, sizeof(in_addr))<0) {
	perror("Error setting up multicast transmit interface");
	return -1;
      }
      
      Ins dins(ip_dst,DumpCollector::ins().portId());
      Sockaddr dst(dins);
      unsigned dlen = dst.sizeofName();
      if (::connect(so, dst.name(), dlen)<0) {
	perror("Binding to destination");
	return -1;
      }
      
      printf("Sending contact info %x/%d to %x/%d\n",
	     ins.address(), ins.portId(),
	     dins.address(), dins.portId());
      
      if (::send(so, &ins, sizeof(ins),0)<0) {
	perror("Sending local info");
	return -1;
      }
    }

    pollfd pfd[1];
    pfd[0].fd = _listen.socket();
    pfd[0].events = POLLIN | POLLERR;
    pfd[0].revents = 0;
    unsigned nfd = 1;
    int tmo = 1000;

    while(poll(pfd,nfd,tmo)>0) {
      Sockaddr name;
      unsigned length = name.sizeofName();
      int s = ::accept(_listen.socket(),name.name(), &length);
      if (s<0)
	printf("Accept failed\n");
      else {
	TSocket _accepted(s);
	printf("-----\n");
	const int sz = 0x8000;
	char buff[sz];
	int  nb;
	while((nb=_accepted.read(buff,sz))>0) {
	  buff[nb]=0;
	  printf("%s",buff);
	}
      }
    }
  }
  return 0;
}
