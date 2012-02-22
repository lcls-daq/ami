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

#include <iostream>
#include <fstream>
#include <QtGui/QApplication>

using namespace Ami;
using namespace std;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <xtc path>\n"
	  "         [-i <interface address>]\n"
	  "         [-s <server mcast group>]\n"
	  "         [-L <user plug-in path>]\n"
	  "         [-o <filename for debugging messages>]\n"
	  "         [-e <filename for error messages>]\n", progname);
}


static void redirect(const char *output, const char *error) {
  if (output) {
    if (freopen(output, "w", stdout) == NULL) {
      perror(output);
      return;
    }
    static ofstream ofile;
    ofile.open(output);
    cout.rdbuf(ofile.rdbuf());
  }
  if (error) {
    if (freopen(error, "w", stderr) == NULL) {
      perror(error);
      return;
    }
    static ofstream efile;
    efile.open(error);
    cerr.rdbuf(efile.rdbuf());
  }
}

int main(int argc, char* argv[]) {
  const char* path = "/reg/d/ana12";
  unsigned interface = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  list<UserModule*> userModules;
  bool testMode = false;
  bool separateWindowMode = false;
  char* outputFile = "/dev/null";
  char* errorFile = NULL;

  QApplication app(argc, argv);
  qRegisterMetaType<Dgram>("Dgram");
  qRegisterMetaType<Pds::TransitionId::Value>("Pds::TransitionId::Value");

  int c;
  while ((c = getopt(argc, argv, "p:i:s:f:o:e:L:TW?h")) != -1) {
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
    case 'o':
      outputFile = optarg;
      break;
    case 'e':
      errorFile = optarg;
      break;
    case 'T':
      testMode = true;
      break;
    case 'W':
      separateWindowMode = true;
      break;
    case '?':
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }

  // Output goes to /dev/null if no other file was specified with -o
  // Error goes to console if no other file was specified with -e
  redirect(outputFile, errorFile);

  // Create ServerManager
  ServerManager srv(interface, serverGroup);

  // Construct features, filter, and factory
  vector<FeatureCache*> features;
  for(unsigned i=0; i<Ami::NumberOfSets; i++) {
    features.push_back(new FeatureCache);
  }
  EventFilter filter(userModules, *features[PostAnalysis]);
  AnalysisFactory factory(features, srv, userModules, filter);

  // Run ServerManager in a background thread
  srv.serve(factory);
  srv.start();

  // Start the DetectorSelect GUI unless separateWindowMode (-W) is chosen.
  QGroupBox* groupBox = NULL;
  Ami::Qt::DetectorSelect* output;
  if (! separateWindowMode) {
    printf("Starting DetectorSelect...\n");
    groupBox = new QGroupBox("Offline");
    output = new Ami::Qt::DetectorSelect("AMO Offline Monitoring", interface, interface, serverGroup, groupBox, true);
    output->show();
    printf("Started DetectorSelect.\n");
  } else {
    printf("NOT Starting DetectorSelect...\n");
  }

  // Start the XtcFileClient inside of the DetectorSelect GUI.
  bool sync = true;
  XtcClient client(features, factory, userModules, filter, sync);
  Ami::Qt::XtcFileClient input(groupBox, client, path, testMode);

  app.exec();

  exit(0);
}
