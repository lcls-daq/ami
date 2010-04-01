#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "ami/app/XtcClient.hh"
#include "ami/app/XtcShmClient.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/server/ServerManager.hh"
#include "ami/app/AnalysisFactory.hh"
#include "ami/service/Ins.hh"

#include "pdsdata/xtc/DetInfo.hh"

using namespace Ami;

typedef Pds::DetInfo DI;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <partitionTag>\n"
	  "          -n <partitionIndex>\n"
	  "          -i <interface>\n"
	  "          -s <server mcast group>\n"
	  "          -c <client mcast group>\n"
	  "          [-f] (offline) [-h] (help)\n", progname);
}


int main(int argc, char* argv[]) {
  int c;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  char* partitionTag = 0;
  int   partitionIndex = 0;
  bool offline=false;

  while ((c = getopt(argc, argv, "?hfp:n:i:s:c:")) != -1) {
    switch (c) {
    case 'f':
      offline=true;
      break;
    case 'i':
      if (optarg[0]<'0' || optarg[0]>'9') {
	int skt = socket(AF_INET, SOCK_DGRAM, 0);
	if (skt<0) {
	  perror("Failed to open socket\n");
	  exit(1);
	}
	ifreq ifr;
	strcpy( ifr.ifr_name, optarg);
	if (ioctl( skt, SIOCGIFADDR, (char*)&ifr)==0)
	  interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
	else {
	  printf("Cannot get IP address for network interface %s.\n",optarg);
	  exit(1);
	}
	printf("Using interface %s (%d.%d.%d.%d)\n",
	       optarg,
	       (interface>>24)&0xff,
	       (interface>>16)&0xff,
	       (interface>> 8)&0xff,
	       (interface>> 0)&0xff);
	close(skt);
      }
      else {
	in_addr inp;
	if (inet_aton(optarg, &inp))
	  interface = ntohl(inp.s_addr);
      }
      break; 
    case 's':
      { in_addr inp;
	if (inet_aton(optarg, &inp))
	  serverGroup = ntohl(inp.s_addr);
	break; }
    case 'p':
      partitionTag = optarg;
      break;
    case 'n':
      partitionIndex = strtoul(optarg,NULL,0);
      break;
    case 'c': 
      break;
    case '?':
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }

  if (!partitionTag || !interface) {
    usage(argv[0]);
    exit(0);
  }

  ServerManager   srv(interface, serverGroup);

  FeatureCache    features;
  AnalysisFactory factory(features, srv);

  XtcClient myClient(features, factory, offline);
  XtcShmClient input(myClient, partitionTag, partitionIndex);

  srv.manage(input);
  srv.serve(factory);
  //  srv.start();  // run in another thread
  srv.routine();  // run in this thread
  //  srv.stop();   // terminate the other thread
  srv.dont_serve();
  return 1;
}
