#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "ami/app/XtcClient.hh"
#include "ami/app/XtcShmClient.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/event/FEEGasDetEnergyReader.hh"
#include "ami/event/EBeamReader.hh"
#include "ami/event/PhaseCavityReader.hh"
#include "ami/event/EpicsXtcReader.hh"
#include "ami/event/ControlXtcReader.hh"
#include "ami/server/ServerManager.hh"
#include "ami/app/AnalysisFactory.hh"
#include "ami/app/Opal1kHandler.hh"
#include "ami/app/AcqWaveformHandler.hh"
#include "ami/service/Ins.hh"

#include "pdsdata/xtc/DetInfo.hh"

using namespace Ami;

typedef Pds::DetInfo DI;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <partitionTag>\n"
	  "          -i <interface>\n"
	  "          -s <server mcast group>\n"
	  "          -c <client mcast group>\n"
	  "          [-f] (offline) [-h] (help)\n", progname);
}


int main(int argc, char* argv[]) {
  int c;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  unsigned clientGroup = 0xefff2001;
  char* partitionTag = 0;
  bool offline=false;

  while ((c = getopt(argc, argv, "?hfp:i:s:c:")) != -1) {
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
    case 'c':
      { in_addr inp;
	if (inet_aton(optarg, &inp))
	  clientGroup = ntohl(inp.s_addr);
	break; }
    case 'p':
      partitionTag = optarg;
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
  XtcShmClient input(myClient, partitionTag);

  myClient.insert(new ControlXtcReader     (features));
  myClient.insert(new FEEGasDetEnergyReader(features));
  myClient.insert(new EBeamReader          (features));
  myClient.insert(new PhaseCavityReader    (features));
  myClient.insert(new EpicsXtcReader       (features));

  myClient.insert(new Opal1kHandler(DI(0,DI::AmoVmi,0,DI::Opal1000,0)));
  myClient.insert(new Opal1kHandler(DI(0,DI::AmoBps,0,DI::Opal1000,0)));
  myClient.insert(new Opal1kHandler(DI(0,DI::AmoBps,0,DI::Opal1000,1)));

  myClient.insert(new AcqWaveformHandler(DI(0,DI::AmoGasdet,0,DI::Acqiris,0)));
  myClient.insert(new AcqWaveformHandler(DI(0,DI::AmoIms   ,0,DI::Acqiris,0)));
  myClient.insert(new AcqWaveformHandler(DI(0,DI::AmoITof  ,0,DI::Acqiris,0)));
  myClient.insert(new AcqWaveformHandler(DI(0,DI::AmoMbes  ,0,DI::Acqiris,0)));
  myClient.insert(new AcqWaveformHandler(DI(0,DI::AmoETof  ,0,DI::Acqiris,0)));

  myClient.insert(new Opal1kHandler     (DI(0,DI::Camp  ,0,DI::Opal1000,0)));
  myClient.insert(new AcqWaveformHandler(DI(0,DI::Camp  ,0,DI::Acqiris,0)));

  srv.manage(input);
  srv.serve(factory);
  //  srv.start();  // run in another thread
  srv.routine();  // run in this thread
  //  srv.stop();   // terminate the other thread
  srv.dont_serve();
  return 1;
}
