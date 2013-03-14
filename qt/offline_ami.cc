#include "ami/app/AmiApp.hh"
#include "ami/app/AnalysisFactory.hh"
#include "ami/app/EventFilter.hh"
#include "ami/app/XtcClient.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/UserModule.hh"
#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/XtcFileClient.hh"
#include "ami/event/EventHandler.hh"
#include "ami/server/AnalysisServerManager.hh"
#include "ami/service/Ins.hh"
#include "ami/service/Semaphore.hh"

#include <iostream>
#include <fstream>
#include <QtGui/QApplication>

using namespace Ami;
using namespace std;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <xtc path>\n"
	  "         [-f <path for save/load files>]\n"
	  "         [-L <user plug-in path>]\n"
	  "         [-o <filename for debugging messages>]\n"
	  "         [-e <filename for error messages>]\n"
          "         [-C <color palette>]    (list from {%s); for example \"mono,jet\")\n"
          "         [-R (full resolution)\n"
	  "         [-l (live read mode)]\n"
          "         [-E (expert mode/movie option)]\n", 
          progname,
          Ami::Qt::ImageColorControl::palette_set().c_str());
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

// This should return a different group for every
// running instance on a single machine, which should
// be sufficient if we are using the lo interface.
static unsigned getLocallyUniqueServerGroup() {
  unsigned pid = getpid();
  pid = pid & 0x0000ffff;
  unsigned ipv4_local_scope = 0xefff0000; // 239.255.0.0/16, see RFC 2365 section 6.1
  unsigned group = ipv4_local_scope | (pid & 0x0000ffff);
  printf("Using server group %x\n",group);
  return group;
}

int main(int argc, char* argv[]) {
  const char* path = "/reg/d";
  unsigned interface = 0x7f000001;
  unsigned serverGroup = getLocallyUniqueServerGroup();
  list<UserModule*> userModules;
  bool testMode = false;
  bool liveReadMode = false;
  bool separateWindowMode = false;
  char* outputFile = NULL;
  char* errorFile = NULL;

  QApplication::setStyle("plastique");
  QApplication app(argc, argv);
  qRegisterMetaType<Dgram>("Dgram");
  qRegisterMetaType<Pds::TransitionId::Value>("Pds::TransitionId::Value");

  int c;
  while ((c = getopt(argc, argv, "p:f:o:e:C:L:lERTW?h")) != -1) {
    switch (c) {
    case 'p':
      path = optarg;
      break;
    case 'C':
      if (!Ami::Qt::ImageColorControl::parse_palette_set(optarg)) {
        usage(argv[0]);
        exit(0);
      }
      break;
    case 'E':
      Ami::Qt::ImageDisplay::enable_movie_option();
      break;
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
    case 'l':
      liveReadMode = true;
      break;
    case 'R':
      Ami::EventHandler::enable_full_resolution(true);
      break;
    case 'T':
      testMode = true;
      path = "/reg/d";
      path = "/reg/d/ana02";
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
  AnalysisServerManager srv(interface, serverGroup);

  // Construct features, filter, and factory
  vector<FeatureCache*> features;
  for(unsigned i=0; i<Ami::NumberOfSets; i++) {
    features.push_back(new FeatureCache);
  }
  EventFilter filter(userModules, *features[PostAnalysis]);
  AnalysisFactory factory(features, srv, userModules, filter);

  // Start the DetectorSelect GUI unless separateWindowMode (-W) is chosen.
  QGroupBox* groupBox = NULL;
  Ami::Qt::DetectorSelect* output;
  if (! separateWindowMode) {
    printf("Starting DetectorSelect...\n");
    groupBox = new QGroupBox("Offline");
    output = new Ami::Qt::DetectorSelect("PS-Mon", interface, interface, serverGroup, groupBox, true);
    output->show();
    printf("Started DetectorSelect.\n");
  } else {
    printf("NOT Starting DetectorSelect...\n");
  }

  // Run ServerManager in a background thread
  Semaphore connect_sem(Semaphore::EMPTY);
  srv.serve(factory,&connect_sem);
  srv.start();
  connect_sem.take();

  // Start the XtcFileClient inside of the DetectorSelect GUI.
  bool sync = true;
  XtcClient client(features, factory, userModules, filter, sync);
  Ami::Qt::XtcFileClient input(groupBox, client, path, testMode, liveReadMode);

  app.exec();

  printf("offline_ami: Application has exited.\n");
  exit(0);
}
