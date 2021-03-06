#include "ami/service/Ins.hh"

#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

using namespace Ami;

unsigned Ins::parse_ip(const char* ipString) {
  unsigned ip = 0;
  struct hostent* h = gethostbyname(ipString);
  if (h) {
    ip = htonl(*(in_addr_t*)h->h_addr_list[0]);
  }
  return ip;
}

unsigned Ins::parse_interface(const char* interfaceString) {
  unsigned interface = parse_ip(interfaceString);
  if (interface == 0) {
    int so = socket(AF_INET, SOCK_DGRAM, 0);
    if (so < 0) {
      perror("Failed to open socket\n");
      return 0;
    }
    ifreq ifr;
    strcpy(ifr.ifr_name, interfaceString);
    int rv = ioctl(so, SIOCGIFADDR, (char*)&ifr);
    close(so);
    if (rv != 0) {
      printf("Cannot get IP address for network interface %s.\n",interfaceString);
      return 0;
    }
    sockaddr_in* saddr = (sockaddr_in*)&(ifr.ifr_addr);
    interface = ntohl(saddr->sin_addr.s_addr);
  }
  printf("Using interface %s (%d.%d.%d.%d)\n",
         interfaceString,
         (interface>>24)&0xff,
         (interface>>16)&0xff,
         (interface>> 8)&0xff,
         (interface>> 0)&0xff);
  return interface;
}

unsigned Ins::default_interface()
{
  size_t hlen=64;
  char hname[hlen];
  if (gethostname(hname,hlen)<0) {
    printf("Default interface lookup failed\n");
    return 0x7f000001;
  }
  struct hostent* h = gethostbyname(hname);
  unsigned v = ntohl( *(in_addr_t*)h->h_addr_list[0] );
  return v;
}
