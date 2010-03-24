#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ami/qt/XtcFileClient.hh"
#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
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
#include "ami/app/XtcClient.hh"
#include "ami/service/Ins.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <QtGui/QApplication>

using namespace Ami;

typedef Pds::DetInfo DI;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -e <experiment name> -p <xtc path>\n"
	  "         [-i <interface address>]\n"
	  "         [-s <server mcast group>]\n"
	  "         [-c <client mcast group>]\n", progname);
}


int main(int argc, char* argv[]) {
  int c;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  unsigned clientGroup = 0xefff2001;
  bool offline=false;
  const char* path = "/reg/d/pcds/amo/offline";

  while ((c = getopt(argc, argv, "?hs:c:f:p:")) != -1) {
    switch (c) {
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
    case 'f':
      Ami::Qt::Path::setBase(optarg);
      break;
    case 'p':
      path = optarg;
      break;
    case '?':
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }

  if (!interface) {
    usage(argv[0]);
    exit(0);
  }

  QApplication app(argc, argv);

  ServerManager   srv(interface, serverGroup);

  FeatureCache    features;
  AnalysisFactory factory(features, srv);

  XtcClient     myClient(features, factory, offline);
  Ami::Qt::XtcFileClient input(myClient, path);

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

  srv.serve(factory);
  srv.start();  // run in another thread
  //  srv.routine();  // run in this thread

  input.show();

  Ami::Qt::DetectorSelect output("AMO Offline Monitoring",interface,serverGroup);
  output.show();

  app.exec();

  srv.stop();   // terminate the other thread
  srv.dont_serve();
  return 1;
}
