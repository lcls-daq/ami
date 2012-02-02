#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "ami/qt/DetectorSelect.hh"
#include "ami/qt/Path.hh"
#include "ami/app/AmiApp.hh"
#include "ami/service/Ins.hh"

#include <QtGui/QApplication>

using namespace Ami;

#if 0
class QtAmiApp {
public:
  static int start(string label, unsigned serverGroup, unsigned ppinterface = 0x7f000001, unsigned interface = 0x7f000001, string loadfile = "");
  static unsigned parse_interface(char* iarg);

};

void QtAmiApp::start(std::string label, unsigned serverGroup, unsigned ppinterface, unsigned interface, std::string loadfile)
{

    FILE* f = fopen(loadfile.c_str(),"r");
    if (f) {
      const int MaxConfigSize = 0x100000;
      char* buffer = new char[MaxConfigSize];
      int size = fread(buffer,1,MaxConfigSize,f);
      fclose(f);
      select->set_setup(buffer,size);
      delete[] buffer;
    }
    else {
      fprintf(stderr, "Unable to open %s\n",loadfile.c_str());
    }
  }
}
#endif

int main(int argc, char **argv) 
{
  unsigned ppinterface = 0x7f000001;
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;
  const char* loadfile = 0;

  for(int i=0; i<argc; i++) {
    if (strcmp(argv[i],"-I")==0) {
      ppinterface = Ins::parse_interface(argv[++i]);
    }
    else if (strcmp(argv[i],"-i")==0) {
      interface = Ins::parse_interface(argv[++i]);
    }
    else if (strcmp(argv[i],"-s")==0) {
      serverGroup = Ins::parse_ip(argv[++i]);
    }
    else if (strcmp(argv[i],"-f")==0) {
      Ami::Qt::Path::setBase(argv[++i]);
    }
    else if (strcmp(argv[i],"-F")==0) {
      loadfile = argv[++i];
    }
  }
  QApplication app(argc, argv);
  Ami::Qt::DetectorSelect* select = new Ami::Qt::DetectorSelect("DAQ Online Monitoring", ppinterface, interface, serverGroup);
  select->show();
  if (loadfile) {
    select->load_setup(loadfile);
  }
  app.exec();
}
