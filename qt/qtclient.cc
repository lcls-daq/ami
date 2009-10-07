#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "ami/qt/DetectorSelect.hh"
#include "ami/service/Ins.hh"

#include <QtGui/QApplication>

using namespace Ami;

int main(int argc, char **argv) 
{
  unsigned interface   = 0x7f000001;
  unsigned serverGroup = 0xefff2000;

  for(int i=0; i<argc; i++) {
    if (strcmp(argv[i],"-i")==0) {
      in_addr inp;
      if (inet_aton(argv[++i], &inp))
	interface = ntohl(inp.s_addr);
    }
    else if (strcmp(argv[i],"-s")==0) {
      in_addr inp;
      if (inet_aton(argv[++i], &inp))
	serverGroup = ntohl(inp.s_addr);
    }
  }

  QApplication app(argc, argv);

  Ami::Qt::DetectorSelect* select = new Ami::Qt::DetectorSelect("AMO Online Monitoring",interface,serverGroup);
  select->show();

  app.exec();

  return 0;
}
