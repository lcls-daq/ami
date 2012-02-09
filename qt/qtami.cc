#include "ami/app/AmiApp.hh"
#include "ami/app/AnalysisFactory.hh"
#include "ami/app/EventFilter.hh"
#include "ami/app/XtcClient.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/UserModule.hh"
#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/XtcFileClient.hh"
#include "ami/server/ServerManager.hh"
#include "ami/service/Ins.hh"

#include <QtGui/QApplication>

using namespace Ami;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <xtc path>\n"
	  "         [-i <interface address>]\n"
	  "         [-s <server mcast group>]\n"
	  "         [-L <user plug-in path>]\n", progname);
}


int main(int argc, char* argv[]) {
  //const char* path = "/reg/d";
  const char* path = "/reg/neh/home/jbarrera/ana01/sxr/sxr13910";
  unsigned interface = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  std::list<UserModule*> userModules;

  QApplication app(argc, argv);

  int c;
  while ((c = getopt(argc, argv, "p:i:s:f:L:?h")) != -1) {
    switch (c) {
    case 'p':
      path = optarg;
      break;
    case 'i':
      interface = Ami::Ins::parse_interface(optarg);
    case 's':
      serverGroup = Ami::Ins::parse_ip(optarg);
    case 'L':
      Ami::AmiApp::load_syms<UserModule,create_m>(userModules, optarg);
      break;
    case 'f':
      Ami::Qt::Path::setBase(optarg); // XXX for ami save files?
      break;
    case '?':
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }

  // Create ServerManager
  ServerManager srv(interface, serverGroup);

  // Construct features, filter, and factory
  std::vector<FeatureCache*> features;
  for(unsigned i=0; i<Ami::NumberOfSets; i++) {
    features.push_back(new FeatureCache);
  }
  EventFilter filter(userModules, *features[PostAnalysis]);
  AnalysisFactory factory(features, srv, userModules, filter);

  // Run ServerManager in a background thread
  srv.serve(factory);
  srv.start();

  // Start the DetectorSelect GUI.
  QGroupBox* groupBox = new QGroupBox("Offline");
  Ami::Qt::DetectorSelect output("AMO Offline Monitoring", interface, interface, serverGroup, groupBox);
  output.show();

  // XXX Why do we pass factory to both ServerManager and XtcClient?
  // XXX If factory is constructed from features, userModules, filter,
  // XXX then why do we have to pass those again to XtcClient?

  // Start the XtcFileClient inside of the DetectorSelect GUI.
  bool offline = true; // XXX true?
  XtcClient myClient(features, factory, userModules, filter, offline);
  Ami::Qt::XtcFileClient input(groupBox, myClient, path);

  app.exec();

  exit(0);
}
