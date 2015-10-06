#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/OffloadEngine.hh"
#include "ami/qt/QtPStack.hh"
#include "ami/service/Ins.hh"
#include "ami/service/DataLock.hh"
#include "ami/qt/QOnline.hh"
#include "ami/event/EpicsXtcReader.hh"

#include <QtGui/QApplication>

#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>

#include <stdio.h>
#include <getopt.h>

static void usage(char* p)
{
  printf("Usage: %s  -s <address> [options]\n"
         "arguments: <interface> = IP address (dot notation) or ethX, defaults to primary interface\n"
         "           <address>   = IP address (dot notation)\n"
         "           <path>      = full file path\n"
         "  Options:\n"
         "-I <interface> : point-to-point interface (cds subnet)\n"
         "-i <interface> : multicast interface (fez subnet), reqd if -s is multicast group\n"
         "-s <address>   : server multicast group or proxy address\n"
         "-n <addresses> : comma-separated list of monshmserver node addresses:\n"
         "                   Connect to the shared memory servers in this list and manage their mapping\n"
         "                   to the DSS node data. The user can dynamically change that mapping from the GUI.\n"
         "                   Note that only one process may control the servers.\n"
         "-p <platform>  : DAQ platform number.  Used in conjunction with -n option above.\n"
         "-f <path>      : default path for load/save operations\n"
         "-A             : attach dialogs rather than popup\n"
         "-F <path>      : file to load initial configuration\n"
         "-C <int>       : color palette choice (0-jette, 1-radiation)\n"
         "-E             : expert mode/movie option\n"
         "-X <path>      : archive path for configuration saves\n"
	 "-Y             : disable synchronous image locking\n"
	 "-Z             : disable image render offload\n"
	 "-a             : <include|only> (EPICS aliases include|only)]\n"
	 ,p);
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

int main(int argc, char **argv) 
{
  unsigned ppinterface;
  unsigned interface  ;
  unsigned serverGroup = 0xefff2000;
  const char* loadfile = 0;
  unsigned platform = 0;
  const char* nodes = 0;
  bool parse_valid = true;

  ppinterface = interface = Ami::Ins::default_interface();

  qInstallMsgHandler(QtAssertHandler);

  for(int i=1; i<argc; i++) {
    if (strcmp(argv[i],"-I")==0) {
      ppinterface = Ami::Ins::parse_interface(argv[++i]);
    }
    else if (strcmp(argv[i],"-i")==0) {
      interface = Ami::Ins::parse_interface(argv[++i]);
    }
    else if (strcmp(argv[i],"-s")==0) {
      serverGroup = Ami::Ins::parse_ip(argv[++i]);
    }
    else if (strcmp(argv[i],"-f")==0) {
      Ami::Qt::Path::setBase(argv[++i]);
    }
    else if (strcmp(argv[i],"-F")==0) {
      loadfile = argv[++i];
    }
    else if (strcmp(argv[i],"-A")==0) {
      Ami::Qt::QtPStack::attach(true);
    }
    else if (strcmp(argv[i],"-C")==0) {
      Ami::Qt::ImageColorControl::parse_palette_set(argv[++i]);
    }
    else if (strcmp(argv[i],"-E")==0) {
      Ami::Qt::ImageDisplay::enable_movie_option();
    }
    else if (strcmp(argv[i],"-Y")==0) {
      Ami::DataLock::disable();
    }
    else if (strcmp(argv[i],"-Z")==0) {
      Ami::Qt::OffloadEngine::disable();
    }
    else if (strcmp(argv[i],"-a")==0) {
      Ami::EpicsXtcReader::use_alias(strcmp(optarg,"only")==0);
    }
    else if (strcmp(argv[i],"-p")==0) {
      char* endptr;
      platform = strtoul(argv[++i],&endptr,0);
      parse_valid &= (*endptr==0);
    }
    else if (strcmp(argv[i],"-n")==0) {
      nodes = argv[++i];
    }
    else if (strcmp(argv[i],"-X")==0) {
      Ami::Qt::Path::setArchive(argv[++i]);
    }
//     else if (strcmp(argv[i],"-h")==0 ||
//              strcmp(argv[i],"-?")==0) {
    else {
      usage(argv[0]);
      exit(1);
    }
  }

  if (!parse_valid) {
    usage(argv[0]);
    exit(-1);
  }

  QApplication app(argc, argv);

  QWidget* onlW = new Ami::Qt::QOnline(nodes,platform);
  ::QObject::connect(onlW, SIGNAL(exit()), &app, SLOT(closeAllWindows()));

  Ami::Qt::DetectorSelect* select = new Ami::Qt::DetectorSelect("DAQ Online Monitoring", ppinterface, interface, serverGroup, onlW, true);
  select->show();
  if (loadfile) {
    select->load_setup(loadfile);
  }
  app.exec();
}
