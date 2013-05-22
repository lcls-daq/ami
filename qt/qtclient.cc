#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/service/Ins.hh"
#include "ami/qt/QOnline.hh"

#include <QtGui/QApplication>

#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>

#include <stdio.h>

static void usage(char* p)
{
  printf("Usage: %s -I <interface> [-i <interface>] -s <address> [-n <addresses] [-f <path> -F <path> -C <int>]\n" \
         "arguments: <interface> = IP address (dot notation) or ethX\n" \
         "           <address>   = IP address (dot notation)\n" \
         "           <path>      = full file path\n" \
         "-p <platform>  : DAQ platform number\n" \
	 "-P <partition> : DAQ partition name\n" \
         "-i <interface> : multicast interface (fez subnet), reqd if -s is multicast group\n" \
         "-s <address>   : server multicast group or proxy address\n" \
         "-n <addresses> : comma-separated list of monshmserver node addresses\n" \
         "-f <path>      : default path for load/save operations\n" \
         "-F <path>      : file to load initial configuration\n" \
         "-C <int>       : color palette choice (0-jette, 1-radiation)" \
         "-E             : expert mode/movie option\n",p);
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
  unsigned ppinterface = 0x7f000001;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  const char* loadfile = 0;
  const unsigned NO_PLATFORM = -1;
  unsigned platform    = NO_PLATFORM;
  const char* partition = 0;
  const char* nodes = 0;

  qInstallMsgHandler(QtAssertHandler);

  for(int i=0; i<argc; i++) {
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
    else if (strcmp(argv[i],"-C")==0) {
      Ami::Qt::ImageColorControl::parse_palette_set(argv[++i]);
    }
    else if (strcmp(argv[i],"-E")==0) {
      Ami::Qt::ImageDisplay::enable_movie_option();
    }
    else if (strcmp(argv[i],"-p")==0) {
      platform = strtoul(argv[++i],NULL,0);
    }
    else if (strcmp(argv[i],"-P")==0) {
      partition = argv[++i];
    }
    else if (strcmp(argv[i],"-n")==0) {
      nodes = argv[++i];
    }
    else if (strcmp(argv[i],"-h")==0 ||
             strcmp(argv[i],"-?")==0) {
      usage(argv[0]);
      exit(1);
    }
  }
  QApplication app(argc, argv);

  QWidget* onlW = new Ami::Qt::QOnline(nodes);
  ::QObject::connect(onlW, SIGNAL(exit()), &app, SLOT(closeAllWindows()));

  Ami::Qt::DetectorSelect* select = new Ami::Qt::DetectorSelect("DAQ Online Monitoring", ppinterface, interface, serverGroup, onlW, true);
  select->show();
  if (loadfile) {
    select->load_setup(loadfile);
  }
  app.exec();
}
