#include "ami/app/CmdLineTools.hh"
#include "ami/app/AmiApp.hh"
#include "ami/app/AnalysisFactory.hh"
#include "ami/app/EventFilter.hh"
#include "ami/app/XtcClientT.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/UserModule.hh"
#include "ami/qt/Client.hh"
#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/OffloadEngine.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/XtcFileClient.hh"
#include "ami/qt/QtPStack.hh"
#include "ami/event/EpicsXtcReader.hh"
#include "ami/event/EventHandler.hh"
#include "ami/event/Calib.hh"
#include "ami/server/AnalysisServerManager.hh"
#include "ami/service/Ins.hh"
#include "ami/service/DataLock.hh"
#include "ami/service/Semaphore.hh"

#include <iostream>
#include <fstream>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>

#include <stdlib.h>

using namespace Ami;
using namespace std;

static void usage(char* progname) {
  fprintf(stderr,
          "Usage: %s [-p <xtc path>]\n"
          "          [-f <path for save/load files>]\n"
          "          [-e <filename for error messages>]\n"
          "          [-o <filename for debugging messages>]\n"
          "          [-l (live read mode)]\n"
          "          [-t (calib test mode)]\n"
          "          [-a <include|only> (EPICS aliases include|only)]\n"
          "          [-w (enable legacy online write pedestal feature)]\n"
          "          [-c <calibdir>]\n"
          "          [-A (attach dialogs rather than popup)]\n"
          "          [-C <color palette>]    (list from {%s); for example \"mono,jet\")\n"
          "          [-D (post detector diagnostics)]\n"
          "          [-E (expert mode/movie option)]\n"
          "          [-L <user plug-in path>]\n"
          "          [-N <threads> (set parallel processing threads, default=1)]\n"
          "          [-O (disable legacy online pedestal corrections)]\n"
          "          [-Q <pixels> (set resolution)]\n"
          "          [-R (set full resolution)]\n"
          "          [-S (use scroll bars)]\n"
          "          [-T (test mode)]\n"
          "          [-X <path> (archive path for configuration saves)]\n"
          "          [-Y (disable synchronous image locking)]\n"
          "          [-Z (disable image render offload)]\n",
          progname,
          Ami::Qt::ImageColorControl::palette_set().c_str());
}


static void QtAssertHandler(QtMsgType type, 
                            const char* msg)
{
  switch(type) {
  case QtDebugMsg:
    fprintf(stderr, "Debug: %s\n", msg); 
    break;
  case QtWarningMsg:
    fprintf(stderr, "Warning: %s\n", msg);
    break;
  case QtCriticalMsg:
    fprintf(stderr, "Critical: %s\n", msg);
    break;
  case QtFatalMsg:
    fprintf(stderr, "Fatal: %s\n", msg);
    __asm("int3");
    abort();
  }
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
  unsigned nclients = 1;
  bool testMode = false;
  bool liveReadMode = false;
  bool separateWindowMode = false;
  char* outputFile = NULL;
  char* errorFile = NULL;
  bool parse_valid = true;
  std::vector<char *> userModulesArgs;

  Ami::Calib::use_offline(true);

  qInstallMsgHandler(QtAssertHandler);

  QApplication::setStyle("plastique");
  QApplication app(argc, argv);
  qRegisterMetaType<Dgram>("Dgram");
  qRegisterMetaType<Pds::TransitionId::Value>("Pds::TransitionId::Value");

  int c;
  while ((c = getopt(argc, argv, "a:p:f:o:e:r:wc:AC:L:N:OlDERQ:StTWX:YZ?h")) != -1) {
    switch (c) {
    case 'p':
      path = optarg;
      break;
    case 'a':
      Ami::EpicsXtcReader::use_alias(strcmp(optarg,"only")==0);
      break;
    case 'A':
      Ami::Qt::QtPStack::attach(true);
      break;
    case 'c':
      Ami::Calib::set_offline_root(optarg);
      break;
    case 'C':
      if (!Ami::Qt::ImageColorControl::parse_palette_set(optarg)) {
        usage(argv[0]);
        exit(0);
      }
      break;
    case 'D':
      Ami::EventHandler::post_diagnostics(true);
      break;
    case 't':
      Ami::Calib::use_offline(false);
      break;
    case 'E':
      Ami::Qt::ImageDisplay::enable_movie_option();
      break;
    case 'L':
      userModulesArgs.push_back(optarg);
      break;
    case 'N':
      nclients = atoi(optarg);
      break;
    case 'O':
      Ami::Calib::use_online(false);
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
    case 'w':
      Ami::Calib::show_write_pedestals(true);
      break;
    case 'R':
      Ami::EventHandler::enable_full_resolution(true);
      break;
    case 'Q':
      { unsigned arg;
        parse_valid &= CmdLineTools::parseUInt(optarg,arg);
        Ami::EventHandler::limit_resolution(arg); }
      break;
    case 'S':
      Ami::Qt::Client::use_scroll_area(true);
      break;
    case 'T':
      testMode = true;
      path = "/reg/d";
      path = "/reg/d/ana02";
      break;
    case 'W':
      separateWindowMode = true;
      break;
    case 'X':
      Ami::Qt::Path::setArchive(optarg);
      break;
    case 'Y':
      Ami::DataLock::disable();
      break;
    case 'Z':
      Ami::Qt::OffloadEngine::disable();
      break;
    case '?':
    case 'h':
    default:
      printf("Unrecognized option %c\n",c);
      usage(argv[0]);
      exit(0);
    }
  }

  if (optind!=argc)
    printf("More parameters than expected [%d/%d]\n",optind,argc);

  parse_valid &= (optind==argc);
  
  if (!parse_valid) {
    usage(argv[0]);
    exit(-1);
  }

//  Consider defaulting resolution to discovered screen size
//   { QDesktopWidget widget;
//     QRect mainScreenSize = widget.availableGeometry(widget.primaryScreen()); 
//     printf("screen size %u x %u\n", mainScreenSize.width(), mainScreenSize.height()); }

  // Output goes to /dev/null if no other file was specified with -o
  // Error goes to console if no other file was specified with -e
  redirect(outputFile, errorFile);

  std::vector<XtcClientT*> clients;
  while(nclients--) {
    //  Create User Modules
    list<UserModule*>& userModules = *new list<UserModule*>;
    for (unsigned i = 0; i < userModulesArgs.size(); i++) {
      Ami::AmiApp::load_syms<UserModule,create_m>(userModules, userModulesArgs[i]);
    }

    // Create ServerManager
    AnalysisServerManager& srv = *new AnalysisServerManager(interface, serverGroup);

    // Construct features, filter, and factory
    vector<FeatureCache*>& features = *new vector<FeatureCache*>;
    for(unsigned i=0; i<Ami::NumberOfSets; i++)
      features.push_back(new FeatureCache);

    EventFilter& filter = *new EventFilter(userModules, *features[PostAnalysis]);
    AnalysisFactory& factory = *new AnalysisFactory(features, srv, userModules, filter);

    // Run ServerManager in a background thread
    Semaphore& connect_sem = *new Semaphore(Semaphore::EMPTY);
    srv.serve(factory,&connect_sem);
    srv.start();
    //    connect_sem.take();

    // Start the XtcFileClient inside of the DetectorSelect GUI.
    bool sync = true;
    //    bool sync = nclients == 0;
    //    bool sync = false;
    
    clients.push_back(new XtcClientT(features, factory, filter, sync));
  }

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

  Ami::Qt::XtcFileClient input(groupBox, clients, path, testMode, liveReadMode);

  app.exec();

  printf("offline_ami: Application has exited.\n");
  exit(0);
}
