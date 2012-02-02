#include <iostream>
#include "ami/app/AmiApp.hh"
#include "ami/qt/XtcFileClient.hh"
#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "ami/service/Ins.hh"
#include <QtGui/QApplication>

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <partitionTag>\n"
	  "         [-d <xtc dir>]\n"
	  "         [-i <interface>]\n"
	  "         [-s <server mcast group>]\n"
	  "         [-L <user plug-in path>]\n", progname);
}


int main(int argc, char* argv[]) {
  char* partitionTag = NULL;
  const char* dir = "/reg/d";
  unsigned interface = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  std::vector<char *> module_names;

  int c;
  while ((c = getopt(argc, argv, "p:d:i:s:L:?h")) != -1) {
    switch (c) {
    case 'p':
      partitionTag = optarg;
      break;
    case 'd':
      dir = optarg;
      break;
    case 'i':
      interface = Ami::Ins::parse_interface(optarg);
    case 's':
      serverGroup = Ami::Ins::parse_ip(optarg);
    case 'L':
      module_names.push_back(optarg);
      break;
    case '?':
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }

  if (!partitionTag) {
    usage(argv[0]);
    exit(0);
  }

  QApplication app(argc, argv);

  Ami::Qt::XtcFileClient client(partitionTag, serverGroup, interface, dir);
  client.show();
  if (fork() == 0) {
    const int partitionIndex = 0;
    const bool sync = true;
    Ami::AmiApp::run(partitionTag, serverGroup, module_names, interface, partitionIndex, sync);
  }
  Ami::Qt::DetectorSelect output("AMO Offline Monitoring", interface, interface, serverGroup);
  output.show();
  app.exec();
  exit(0);
}
