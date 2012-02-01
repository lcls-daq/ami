#include <iostream>
#include "ami/app/AmiApp.hh"
#include "ami/qt/XtcFileClient.hh"
#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include <QtGui/QApplication>

using namespace Ami;
using namespace std;

typedef Pds::DetInfo DI;

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -e <experiment name> -p <xtc path>\n"
	  "         [-i <interface address>]\n"
	  "         [-s <server mcast group>]\n"
	  "         [-L <user plug-in path>]\n", progname);
}


int main(int argc, char* argv[]) {
  int c;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  bool offline=false;
  //const char* path = "/reg/d/pcds/amo/offline";
  const char* path = "/reg/d/ana01/xpp";

  //  plug-in modules
  std::vector<char *> module_names;

  while ((c = getopt(argc, argv, "?hs:L:f:p:")) != -1) {
    switch (c) {
    case 's':
      serverGroup = AmiApp::parse_ip(optarg);
    case 'L':
      module_names.push_back(optarg);
      break;
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

  int partitionIndex = 0;
  char* partitionTag = getenv("USER");
  if (partitionTag == NULL) {
    cerr << "The USER environment variable must be set!" << endl;
    exit(1);
  }

  Ami::Qt::XtcFileClient client(path, interface, serverGroup);
  client.show();

  if (fork() == 0) {
    AmiApp::run(partitionTag, serverGroup, module_names, interface, partitionIndex, offline);
    cout << "Started AmiApp::run..." << endl;
  }

  cout << "starting DetectorSelect..." << endl;
  Ami::Qt::DetectorSelect output("AMO Offline Monitoring", interface, interface, serverGroup);
  output.show();

  cout << "doing app.exec()..." << endl;
  app.exec();
  cout << "did app.exec()..." << endl;

  exit(0);
}
