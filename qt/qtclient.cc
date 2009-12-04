#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "ami/service/Ins.hh"

#include <QtGui/QApplication>

using namespace Ami;

int main(int argc, char **argv) 
{
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;

  for(int i=0; i<argc; i++) {
    if (strcmp(argv[i],"-i")==0) {
      char* iarg = argv[++i];
      if (iarg[0]<'0' || iarg[0]>'9') {
	int skt = socket(AF_INET, SOCK_DGRAM, 0);
	if (skt<0) {
	  perror("Failed to open socket\n");
	  exit(1);
	}
	ifreq ifr;
	strcpy( ifr.ifr_name, iarg);
	if (ioctl( skt, SIOCGIFADDR, (char*)&ifr)==0)
	  interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
	else {
	  printf("Cannot get IP address for network interface %s.\n",iarg);
	  exit(1);
	}
	printf("Using interface %s (%d.%d.%d.%d)\n",
	       iarg,
	       (interface>>24)&0xff,
	       (interface>>16)&0xff,
	       (interface>> 8)&0xff,
	       (interface>> 0)&0xff);
	close(skt);
      }
      else {
	in_addr inp;
	if (inet_aton(iarg, &inp))
	  interface = ntohl(inp.s_addr);
      }
    }
    else if (strcmp(argv[i],"-s")==0) {
      in_addr inp;
      if (inet_aton(argv[++i], &inp))
	serverGroup = ntohl(inp.s_addr);
    }
    else if (strcmp(argv[i],"-f")==0) {
      Ami::Qt::Path::setBase(argv[++i]);
    }
  }

  QApplication app(argc, argv);

  Ami::Qt::DetectorSelect* select = new Ami::Qt::DetectorSelect("AMO Online Monitoring",interface,serverGroup);
  select->show();

  app.exec();

  return 0;
}
