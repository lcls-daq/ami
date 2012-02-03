#include <iostream>
#include "ami/app/AmiApp.hh"
#include "ami/qt/XtcFileClient.hh"
#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "ami/service/Ins.hh"
#include "ami/service/Routine.hh"
#include "ami/service/Task.hh"
#include <QtGui/QApplication>
#include <QtGui/QGroupBox>

static void usage(char* progname) {
  fprintf(stderr,
	  "Usage: %s -p <partitionTag>\n"
	  "         [-d <xtc dir>]\n"
	  "         [-i <interface>]\n"
	  "         [-s <server mcast group>]\n"
	  "         [-L <user plug-in path>]\n", progname);
}

class AmiRoutine : public Ami::Routine {
private:
  char* _partitionTag;
  unsigned _serverGroup;
  std::vector<char *>& _module_names;
  unsigned _interface;

public:
  AmiRoutine(char* partitionTag,
             unsigned serverGroup,
             std::vector<char *>& module_names,
             unsigned interface) :
    _partitionTag(partitionTag),
    _serverGroup(serverGroup),
    _module_names(module_names),
    _interface(interface) {
  }

  void routine() {
    const int partitionIndex = 0;
    const bool sync = true;
    std::cout << "AmiApp::run starting..." << std::endl;
    Ami::AmiApp::run(_partitionTag, _serverGroup, _module_names, _interface, partitionIndex, sync);
    std::cout << "AmiApp::run done..." << std::endl;
  }
};

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

  // Start AmiApp in the background.
  Ami::Task* amiTask = new Ami::Task(Ami::TaskObject("AmiThread"));
  AmiRoutine amiRoutine = AmiRoutine(partitionTag, serverGroup, module_names, interface);
  amiTask->call(&amiRoutine);

  // Start the DetectorSelect GUI.
  QApplication app(argc, argv);
  QGroupBox* groupBox = new QGroupBox("Offline");
  Ami::Qt::DetectorSelect output("AMO Offline Monitoring", interface, interface, serverGroup, groupBox);
  output.show();

  // Start the XtcFileClient inside of the DetectorSelect GUI.
  Ami::Qt::XtcFileClient client(groupBox, partitionTag, serverGroup, interface, dir);
  //client.show();
  app.exec();

  amiTask->destroy();
  exit(0);
}
