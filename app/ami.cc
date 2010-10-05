#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dlfcn.h>

#include "ami/app/XtcClient.hh"
#include "ami/app/XtcShmClient.hh"
#include "ami/app/AnalysisFactory.hh"

#include "ami/server/ServerManager.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/UserAnalysis.hh"
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
	  "          -L <user plug-in path>\n"
	  "          [-f] (offline) [-h] (help)\n", progname);
}


int main(int argc, char* argv[]) {
  int c;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0;
  char* partitionTag = 0;
  int   partitionIndex = 0;
  bool offline=false;
  //  plug-in module
  UserAnalysis* user = 0;
  create_t*  create_user  = 0;
  destroy_t* destroy_user = 0;

  while ((c = getopt(argc, argv, "?hfp:n:i:s:L:")) != -1) {
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
    case 'L': 
      {
	void* handle = dlopen(optarg, RTLD_LAZY);
	if (!handle) break;

	// reset errors
	const char* dlsym_error;
	dlerror();

	// load the symbols
	create_t* c_user = (create_t*) dlsym(handle, "create");
	if ((dlsym_error = dlerror())) {
	  fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
	  break;
	}

	dlerror();
	destroy_t* d_user = (destroy_t*) dlsym(handle, "destroy");
	if ((dlsym_error = dlerror())) {
	  fprintf(stderr,"Cannot load symbol destroy: %s\n",dlsym_error);
	  break;
	}

	create_user  = c_user;
	destroy_user = d_user;
      }
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

  if (create_user)
    user = create_user();

  FeatureCache    features;
  AnalysisFactory factory(features, srv, user);

  XtcClient myClient(features, factory, user, offline);
  XtcShmClient input(myClient, partitionTag, partitionIndex);

  srv.manage(input);
  srv.serve(factory);
  //  srv.start();  // run in another thread
  srv.routine();  // run in this thread
  //  srv.stop();   // terminate the other thread
  srv.dont_serve();

  if (user)
    destroy_user(user);

  return 1;
}
